// Copyright(C) 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <array>
#include <assert.h>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)
#include <numeric>
#include <stddef.h>
#include <string>

#include "Ioss_ChainGenerator.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementTopology.h"
#include "Ioss_FaceGenerator.h"
#include "Ioss_Property.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_Utils.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"
#include "robin_hash.h"
#include "tokenize.h"

// ========================================================================

namespace {
  template <typename INT> using front_t = std::vector<std::pair<INT, int>>;
  using connectivity_t                  = std::vector<std::array<const Ioss::Face *, 6>>;

  int debug = 0;

  // 0-based face
  int hex_opposite_side(int side)
  {
    switch (side) {
    case 0: return 2;
    case 1: return 3;
    case 2: return 0;
    case 3: return 1;
    case 4: return 5;
    case 5: return 4;
    }
    return -1;
  }

  Ioss::NameList get_adjacent_blocks(Ioss::Region &region, const std::string &surface_list)
  {
    Ioss::NameList adjacent_blocks;
    if (surface_list == "ALL") {
      const Ioss::SideSetContainer &fss = region.get_sidesets();
      for (const auto &fs : fss) {
        // Save a list of all blocks that are adjacent to the surfaces...
        Ioss::NameList blocks;
        fs->block_membership(blocks);
        for (const auto &block : blocks) {
          adjacent_blocks.push_back(block); // May have duplicates at this point.
        }
      }
    }
    else {
      auto selected_surfaces = Ioss::tokenize(surface_list, ",");
      for (const auto &surface : selected_surfaces) {
        auto *sset = region.get_sideset(surface);
        if (sset != nullptr) {
          // Save a list of all blocks that are adjacent to the surfaces...
          Ioss::NameList blocks;
          sset->block_membership(blocks);
          for (const auto &block : blocks) {
            adjacent_blocks.push_back(block); // May have duplicates at this point.
          }
        }
        else {
          fmt::print("\nWARNING: Surface '{}' does not exist in this model.\n", surface);
        }
      }
    }

    Ioss::Utils::uniquify(adjacent_blocks);
    return adjacent_blocks;
  }

  template <typename INT>
  void get_line_front(Ioss::SideSet *fs, const Ioss::ElementBlock *block,
                      Ioss::chain_t<INT> &element_chains, front_t<INT> &front)
  {
    const auto    &adj_block_name = block->name();
    Ioss::NameList blocks;
    fs->block_membership(blocks);
    for (const auto &fs_block : blocks) {
      if (fs_block == adj_block_name) {
        // This faceset has some elements that are in `adj_block_name` -- put those in the `front`
        // list. Get list of "sides" in this faceset...
        std::vector<INT> element_side;
        assert(fs->side_block_count() == 1);
        const auto *fb = fs->get_block(0);
        fb->get_field_data("element_side_raw", element_side);

        // Mark each element so we know it is on the sideset(s)
        for (size_t i = 0; i < element_side.size(); i += 2) {
          auto element = element_side[i];
          if (block->contains(element)) {
            if (element_chains[element - 1] == Ioss::chain_entry_t<INT>()) {
              int side                    = static_cast<int>(element_side[i + 1]); // 1-based sides
              element_chains[element - 1] = Ioss::chain_entry_t<INT>{element, 0};
              front.push_back(std::make_pair(element, side));
              if (debug & 16) {
                fmt::print("Putting element {}, side {} in front.\n", element, side);
              }
            }
          }
        }
      }
    }
  }

  template <typename INT>
  front_t<INT> get_line_front(Ioss::Region &region, const Ioss::ElementBlock *block,
                              Ioss::chain_t<INT> &element_chains, const std::string &surface_list)
  {
    front_t<INT> front;

    // Since lines can not cross element blocks, we can process everything a block at a time.
    assert(block != nullptr);
    assert(block->topology()->shape() == Ioss::ElementShape::HEX);

    if (surface_list == "ALL") {
      const Ioss::SideSetContainer &fss = region.get_sidesets();
      for (const auto &fs : fss) {
        get_line_front(fs, block, element_chains, front);
      }
    }
    else {
      auto selected_surfaces = Ioss::tokenize(surface_list, ",");
      for (const auto &surface : selected_surfaces) {
        auto *sset = region.get_sideset(surface);
        if (sset != nullptr) {
          get_line_front(sset, block, element_chains, front);
        }
      }
    }
    return front;
  }

  void generate_face_connectivity(const Ioss::FaceUnorderedSet &faces, int offset,
                                  connectivity_t &face_connectivity)
  {
    for (const auto &face : faces) {
      for (int i = 0; i < face.element_count(); i++) {
        auto element                     = face.element[i] / 10 - offset;
        auto side                        = face.element[i] % 10; // 0-based side
        face_connectivity[element][side] = &face;
      }
    }

    if (debug & 16) {
      fmt::print("\n-----------------------------\n");
      int l = 1;
      for (size_t i = 0; i < face_connectivity.size(); i++) {
        for (size_t j = 0; j < 6; j++) {
          const auto *face = face_connectivity[i][j];
          assert(face != nullptr);
          int  k       = (face->element_count() > 1 && face->element[0] / 10 - offset != i) ? 1 : 0;
          auto element = face->element[k] / 10;
          auto side    = face->element[k] % 10;
          assert(side == j);
          if (face->element_count() > 1) {
            fmt::print(
                "[{:3}] Element {}, Side {}/{} is Face {}.\tAdjacent to Element {}, Side {}.\n",
                l++, element, side, j, face->hashId_, face->element[1 - k] / 10,
                face->element[1 - k] % 10);
          }
          else {
            fmt::print("[{:3}] Element {}, Side {}/{} is Face {}.\n", l++, element, side, j,
                       face->hashId_);
          }
        }
      }
    }
  }
} // namespace

namespace Ioss {

  template IOSS_EXPORT Ioss::chain_t<int> generate_element_chains(Ioss::Region &region,
                                                                  const std::string &, int, int);
  template IOSS_EXPORT                    Ioss::chain_t<int64_t>
  generate_element_chains(Ioss::Region &region, const std::string &, int, int64_t);

  template <typename INT>
  Ioss::chain_t<INT> generate_element_chains(Ioss::Region &region, const std::string &surface_list,
                                             int debug_level, INT /*dummy*/)
  {
    region.get_database()->progress(__func__);

    debug        = debug_level;
    size_t numel = region.get_property("element_count").get_int();

    // Determine which element block(s) are adjacent to the faceset specifying "lines"
    // The `adjacent_blocks` contains the names of all element blocks that are adjacent to the
    // surface(s) that specify the faces at the 'root' of the lines...
    Ioss::NameList adjacent_block_names = get_adjacent_blocks(region, surface_list);
    if (adjacent_block_names.empty()) {
      fmt::print("WARNING: No surfaces in the model matched the input surface list ({}).\n\tNo "
                 "chains will be generated.\n",
                 surface_list);
    }

    // Get the EB* corresponding to the EB names...
    Ioss::ElementBlockContainer adjacent_blocks;
    adjacent_blocks.reserve(adjacent_block_names.size());
    for (const auto &blk_name : adjacent_block_names) {
      auto *eb = region.get_element_block(blk_name);
      assert(eb != nullptr);
      if (eb->topology()->shape() != Ioss::ElementShape::HEX) {
        fmt::print("Skipping Element Block {}; it does not contain HEX elements.\n", blk_name);
      }
      else {
        adjacent_blocks.push_back(eb);
      }
    }

    // Generate the faces for use later... (only generate on the blocks touching the front)
    Ioss::FaceGenerator face_generator(region);
    face_generator.generate_block_faces(adjacent_blocks, (INT)0, true);
    region.get_database()->progress("\tAfter generate_block_faces");

    Ioss::chain_t<INT> element_chains(numel);
    for (const auto *block : adjacent_blocks) {
      // Get the offset into the element_chains vector...
      auto offset = block->get_offset() + 1;
      auto count  = block->entity_count();

      auto front = get_line_front(region, block, element_chains, surface_list);
      if (front.empty()) {
        continue;
      }

      // We want a vector giving us the Face for each face of each element in the block...
      connectivity_t face_connectivity(count);
      generate_face_connectivity(face_generator.faces(block), static_cast<int>(offset),
                                 face_connectivity);
      face_generator.clear(block);

      // For each face on the "front" (at the beginning the boundary sideset faces)
      // Set `element_chains` to the `face` "ID"
      // We are only working on the elements that are in the current block...
      front_t<INT> next_front;
      while (!front.empty()) {
        if (debug & 16) {
          fmt::print("\n----------------------\n");
        }
        next_front.reserve(front.size());
        for (auto &element_side : front) {
          auto element = element_side.first;
          auto side    = element_side.second - 1;

          auto opp_side = hex_opposite_side(side);
          assert(opp_side >= 0);
          auto *opp_face = face_connectivity[element - offset][opp_side];
          // See if there is an element attached to the `opp_side`
          if (opp_face->element_count() > 1) {
            // Determine which is current element and which is adjacent element...
            int  index       = (opp_face->element[0] / 10 ==
                         static_cast<typename decltype(opp_face->element)::value_type>(element))
                                   ? 1
                                   : 0;
            auto nxt_element = opp_face->element[index] / 10;
            auto nxt_side    = opp_face->element[index] % 10;
            if (element_chains[nxt_element - 1] == Ioss::chain_entry_t<INT>()) {
              element_chains[nxt_element - 1] = element_chains[element - 1];
              element_chains[nxt_element - 1].link++;
              if (debug & 16) {
                fmt::print("At element {}, side {} -- Next in chain is element {}, side {}\n",
                           element, side, nxt_element, nxt_side);
              }
              next_front.push_back(
                  std::make_pair(static_cast<INT>(nxt_element), static_cast<int>(nxt_side + 1)));
            }
            else {
              if (debug & 16) {
                fmt::print("At element {}, side {} -- Termination of chain {} of size {}.\n",
                           element, side, element_chains[element - 1].element,
                           element_chains[element - 1].link + 1);
              }
            }
          }
          else {
            if (debug & 16) {
              fmt::print("At element {}, side {} -- Termination of chain {} of size {}.\n", element,
                         side, element_chains[element - 1].element,
                         element_chains[element - 1].link + 1);
            }
          }
        }
        std::swap(front, next_front);
        next_front.clear();
      }
    } // End of block loop
    region.get_database()->progress("\tAfter generating chains");
    return element_chains;
  }
} // namespace Ioss
