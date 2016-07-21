#ifndef DIY_DETAIL_ALGORITHMS_SORT_HPP
#define DIY_DETAIL_ALGORITHMS_SORT_HPP

#include <functional>
#include <algorithm>

namespace diy
{

namespace detail
{

template<class Block, class T, class Cmp>
struct SampleSort
{
    typedef         std::vector<T>      Block::*ValuesVector;
    struct Sampler;
    struct Exchanger;

                    SampleSort(ValuesVector values_, ValuesVector samples_, const Cmp& cmp_, size_t num_samples_):
                        values(values_), samples(samples_),
                        cmp(cmp_), num_samples(num_samples_)                    {}

    Sampler         sample() const                                              { return Sampler(values, samples, cmp, num_samples); }
    Exchanger       exchange() const                                            { return Exchanger(values, samples, cmp); }

    static void     dequeue_values(std::vector<T>& v, const ReduceProxy& rp, bool skip_self = true)
    {
        int k_in  = rp.in_link().size();

        //printf("dequeue_values(): gid=%d, round=%d; v.size()=%lu\n", rp.gid(), rp.round(), v.size());

        if (detail::is_default< Serialization<T> >::value)
        {
            // add up sizes
            size_t sz = 0;
            size_t end = v.size();
            for (int i = 0; i < k_in; ++i)
            {
                //printf("    incoming size from %d: %lu\n", rp.in_link().target(i).gid, sz);
                if (skip_self && rp.in_link().target(i).gid == rp.gid()) continue;
                MemoryBuffer& in = rp.incoming(rp.in_link().target(i).gid);
                sz += in.size() / sizeof(T);
            }
            //printf("    incoming size: %lu\n", sz);
            v.resize(end + sz);

            for (int i = 0; i < k_in; ++i)
            {
                if (skip_self && rp.in_link().target(i).gid == rp.gid()) continue;
                MemoryBuffer& in = rp.incoming(rp.in_link().target(i).gid);
                size_t sz = in.size() / sizeof(T);
                T* bg = (T*) &in.buffer[0];
                std::copy(bg, bg + sz, &v[end]);
                end += sz;
            }
        } else
        {
            for (int i = 0; i < k_in; ++i)
            {
                if (skip_self && rp.in_link().target(i).gid == rp.gid()) continue;
                MemoryBuffer& in = rp.incoming(rp.in_link().target(i).gid);
                while(in)
                {
                    T x;
                    diy::load(in, x);
#if __cplusplus > 199711L           // C++11
                    v.emplace_back(std::move(x));
#else
                    v.push_back(x);
#endif
                }
            }
        }
        //printf("    v.size()=%lu\n", v.size());
    }

    ValuesVector    values;
    ValuesVector    samples;
    Cmp             cmp;
    size_t          num_samples;
};

template<class Block, class T, class Cmp>
struct SampleSort<Block,T,Cmp>::Sampler
{
                    Sampler(ValuesVector values_, ValuesVector dividers_, const Cmp& cmp_, size_t num_samples_):
                        values(values_), dividers(dividers_), cmp(cmp_), num_samples(num_samples_)    {}

    void            operator()(void* b_, const ReduceProxy& srp, const RegularSwapPartners& partners) const
    {
        Block* b = static_cast<Block*>(b_);

        int k_in  = srp.in_link().size();
        int k_out = srp.out_link().size();

        std::vector<T> samples;

        if (k_in == 0)
        {
            // draw random samples
            for (size_t i = 0; i < num_samples; ++i)
                samples.push_back((b->*values)[std::rand() % (b->*values).size()]);
        } else
            dequeue_values(samples, srp, false);

        if (k_out == 0)
        {
            // pick subsamples that separate quantiles
            std::sort(samples.begin(), samples.end(), cmp);
            std::vector<T>  subsamples(srp.nblocks() - 1);
            int step = samples.size() / srp.nblocks();       // NB: subsamples.size() + 1
            for (size_t i = 0; i < subsamples.size(); ++i)
                subsamples[i] = samples[(i+1)*step];
            (b->*dividers).swap(subsamples);
        }
        else
        {
            for (int i = 0; i < k_out; ++i)
            {
                MemoryBuffer& out = srp.outgoing(srp.out_link().target(i));
                save(out, &samples[0], samples.size());
            }
        }
    }

    ValuesVector    values;
    ValuesVector    dividers;
    Cmp             cmp;
    size_t          num_samples;
};

template<class Block, class T, class Cmp>
struct SampleSort<Block,T,Cmp>::Exchanger
{
                    Exchanger(ValuesVector values_, ValuesVector samples_, const Cmp& cmp_):
                        values(values_), samples(samples_), cmp(cmp_)       {}

    void            operator()(void* b_, const ReduceProxy& rp) const
    {
        Block* b = static_cast<Block*>(b_);

        if (rp.round() == 0)
        {
            // enqueue values to the correct locations
            for (size_t i = 0; i < (b->*values).size(); ++i)
            {
                int to = std::lower_bound((b->*samples).begin(), (b->*samples).end(), (b->*values)[i], cmp) - (b->*samples).begin();
                rp.enqueue(rp.out_link().target(to), (b->*values)[i]);
            }
            (b->*values).clear();
        } else
        {
            dequeue_values((b->*values), rp, false);
            std::sort((b->*values).begin(), (b->*values).end(), cmp);
        }
    }

    ValuesVector    values;
    ValuesVector    samples;
    Cmp             cmp;
};

}

}

#endif
