namespace diy
{
    struct Master::IExchangeInfoDUD: public IExchangeInfo
    {
        struct Message
        {
            std::array<int,2>   msg;
            mpi::request        request;
        };

      using IExchangeInfo::IExchangeInfo;

      inline bool       all_done() override;                             // get global all done status
      inline void       add_work(int work) override;                     // add work to global work counter
      inline void       control() override;
      inline void       update_subtree(int diff);
      inline bool       process_work_update();
      inline void       check_for_abort();
      inline void       abort(int trial);
      inline void       process_done(int source, int trial);
      inline void       reset_child_confirmations();
      int               right_bit(int x) const                  { return ((x ^ (x-1)) + 1) >> 1; }

      void              send(int rk, int  type, int  x)
      {
          inflight_.emplace_back();
          Message& m = inflight_.back();
          m.msg[0] = type;
          m.msg[1] = x;
          m.request = comm.issend(rk, tags::iexchange, m.msg);
          log->trace("[{}] Sending to {}, type = {}, x = {}", comm.rank(), rk, type, x);
      }
      void              recv(int rk, int& type, int& x)
      {
          std::array<int,2> msg;
          comm.recv(rk, tags::iexchange, msg);
          type = msg[0];
          x = msg[1];
          log->trace("[{}] Received from {}, type = {}, x = {}, msg = {}", comm.rank(), rk, type, x);
      }

      inline bool       nudge();
      int               parent() const                          { return comm.rank() & (comm.rank() - 1); }     // flip the last 1 to 0
      inline void       signal_children(int tag, int x);
      bool              incomplete() const                      { return subtree_work_ > 0 || !inflight_.empty(); }
      bool              stale() const                           { return subtree_work_ != last_subtree_work_message_ || local_work_ != last_local_work_message_; }

      struct type       { enum {
                                    work_update = 0,
                                    done,
                                    abort
                                }; };
      std::unordered_map<int, bool>       done;                 // gid -> done

      int                                 local_work_   = 0, last_local_work_message_   = 0;
      int                                 subtree_work_ = 0, last_subtree_work_message_ = 0;
      int                                 down_up_down_ = 0;

      std::list<Message>                  inflight_;
      int                                 last_trial_ = -1;
      int                                 child_confirmations = -1;

      // debug
      bool                                first_dud = true;
      using IExchangeInfo::prof;
    };
}


// get global all done status
bool
diy::Master::IExchangeInfoDUD::
all_done()
{
    if (down_up_down_ == 3)
        while (!inflight_.empty()) nudge();     // make sure that all the messages are propagated before we finish
                                                // if we've decided that we are done, the only outstanding messages
                                                // can be the done signals to children; nothing else should be in-flight

    return down_up_down_ == 3;
}

// add arbitrary units of work to global work counter
void
diy::Master::IExchangeInfoDUD::
add_work(int work)
{
    int cur_local_work = local_work_;
    local_work_ += work;
    assert(local_work_ >= 0);

    log->trace("[{}] Adding work: work = {}, local_work = {}, cur_local_work = {}", comm.rank(), work, local_work_, cur_local_work);

    if ((cur_local_work == 0) ^ (local_work_ == 0))     // changed from or to zero
    {
        int diff    = (local_work_ - last_local_work_message_);
        update_subtree(diff);
        last_local_work_message_ = local_work_;
    }
}

void
diy::Master::IExchangeInfoDUD::
update_subtree(int diff)
{
    int cur_subtree_work = subtree_work_;
    subtree_work_ += diff;
    log->debug("[{}] Updating subtree: diff = {}, subtree_work_ = {}", comm.rank(), diff, subtree_work_);
    assert(subtree_work_ >= 0);

    if ((cur_subtree_work == 0) ^ (subtree_work_ == 0))     // changed from or to zero
    {
        if (comm.rank() != 0)
        {
            int subtree_diff = (subtree_work_ - last_subtree_work_message_);
            log->debug("[{}] Sending subtree update: diff = {}, subtree_diff = {}", comm.rank(), diff, subtree_diff);
            send(parent(), type::work_update, subtree_diff);
            last_subtree_work_message_ = subtree_work_;
            if (down_up_down_ == 1)
                abort(last_trial_);
            else if (down_up_down_ == 2)
                log->warn("[{}] Enqueueing work update after finishing, diff = {}", comm.rank(), subtree_diff);
                // This is Ok in general: if this happens, somebody else must abort this trial.
            else if (down_up_down_ == 3)
                log->critical("[{}] Enqueueing work update after all done, diff = {}", comm.rank(), subtree_diff);
        } else
        {
            assert(down_up_down_ < 2);
            down_up_down_ = 0;      // if we are updating work on the root, definitely abort the down-up-down protocol
        }
    }
}

void
diy::Master::IExchangeInfoDUD::
control()
{
    mpi::optional<mpi::status> ostatus = comm.iprobe(mpi::any_source, tags::iexchange);
    while(ostatus)
    {
        int t, x;
        recv(ostatus->source(), t, x);
        if (t == type::work_update)
        {
            // x = diff
            log->debug("[{}] subtree update request from {}, diff = {}", comm.rank(), ostatus->source(), x);
            update_subtree(x);      // for now propagates up the tree verbatim
        } else if (t == type::abort)
        {
            // x = trial
            assert(x >= -1);
            abort(x);
        } else if (t == type::done)
        {
            process_done(ostatus->source(), x);
        }
        ostatus = comm.iprobe(mpi::any_source, tags::iexchange);
    }

    // initiate down-up-down protocol
    if (subtree_work_ == 0 && comm.rank() == 0 && down_up_down_ == 0)
    {
        // debug
        if (first_dud)
        {
            prof >> "iexchange-control";        // consensus-time cannot nest in iexchange-control
            prof << "consensus-time";
            prof << "iexchange-control";
            first_dud = false;
        }

        down_up_down_ = 1;
        reset_child_confirmations();
        if (child_confirmations)
        {
            signal_children(type::done, ++last_trial_);
            log->info("Initiated down-up-down, trial = {}", last_trial_);
        } else // no children
            down_up_down_ = 3;
    }

    while(nudge());
}

void
diy::Master::IExchangeInfoDUD::
abort(int trial)
{
    if (down_up_down_ == 0) // already aborted
        return;

    if (trial != last_trial_)
        return;

    log->warn("[{}] aborting trial {}", comm.rank(), trial);
    assert(trial >= 0);

    down_up_down_ = 0;

    if (comm.rank() != 0)
    {
        send(parent(), type::abort, trial);    // propagate abort
        if (down_up_down_ >= 2)
            log->critical("[{}] sending abort after done", comm.rank());
        last_trial_ = -1;       // all future aborts for this trial will be stale
    }
}

void
diy::Master::IExchangeInfoDUD::
process_done(int source, int trial)
{
    if (trial < -1)
    {
        log->critical("[{}] done with source = {}, trial = {}", comm.rank(), source, trial);
        assert(trial >= -1);
    }

    while(nudge());     // clear up finished requests: this is necessary since requests may have been received;
                        // we are now getting responses to them; but they are still listed in our inflight_

    if (source == parent())
    {
        if (trial == last_trial_)       // confirmation that we are done
        {
            assert(down_up_down_ == 2);
            log->info("[{}] received done confirmation from parent, trial = {}; incomplete = {}, subtree = {}, stale = {}",
                      comm.rank(), trial, incomplete(), subtree_work_, stale());
            down_up_down_ = 3;
            assert(!incomplete() && !stale());
        } else
        {
            last_trial_ = trial;
            down_up_down_ = 1;

            // check that there are no changes
            if (incomplete() || stale())
                abort(trial);
        }

        // pass the message to children
        if (down_up_down_ > 0)
        {
            reset_child_confirmations();
            if (child_confirmations)
            {
                log->info("[{}] signalling done to children, trial = {}", comm.rank(), trial);
                signal_children(type::done, trial);
            }
            else if (down_up_down_ < 2)     // no children, signal back to parent right away, unless it was the final done
            {
                down_up_down_ = 2;
                log->info("[{}] signalling done to parent (1), trial = {}, incomplete = {}", comm.rank(), trial, incomplete());
                send(parent(), type::done, trial);
            }
        }
    } else // signal from a child
    {
        if (trial == last_trial_)
        {
            int child_mask = right_bit(source);
            child_confirmations &= ~child_mask;
            if (child_confirmations == 0)       // done
            {
                if (comm.rank() != 0)
                {
                    if (incomplete() || stale())        // heard from all the children, but there is something incomplete
                        abort(trial);
                    else
                    {
                        down_up_down_ = 2;
                        log->info("[{}] signalling done to parent (2), trial = {}, incomplete = {}", comm.rank(), trial, incomplete());
                        send(parent(), type::done, trial);
                    }
                }
                else if (down_up_down_ == 1)
                {
                    log->info("[{}] received done confirmation from children at root, trial = {}", comm.rank(), trial);
                    // initiate final down
                    down_up_down_ = 3;
                    signal_children(type::done, trial);
                }
            }
        } // else stale trial confirmation, drop
    }
}

void
diy::Master::IExchangeInfoDUD::
reset_child_confirmations()
{
    child_confirmations = 0;
    int child_mask = 1;
    int child = child_mask | comm.rank();
    while (child != comm.rank() && child < comm.size())
    {
        child_confirmations |= child_mask;

        child_mask <<= 1;
        child = child_mask | comm.rank();
    }
}

void
diy::Master::IExchangeInfoDUD::
signal_children(int tag, int x)
{
    int child_mask = 1;
    int child = child_mask | comm.rank();
    while (child != comm.rank() && child < comm.size())
    {
        send(child, tag, x);

        child_mask <<= 1;
        child = child_mask | comm.rank();
    }
}

bool
diy::Master::IExchangeInfoDUD::
nudge()
{
  bool success = false;
  for (auto it = inflight_.begin(); it != inflight_.end();)
  {
    mpi::optional<mpi::status> ostatus = it->request.test();
    if (ostatus)
    {
      success = true;
      it = inflight_.erase(it);
    }
    else
      ++it;
  }
  return success;
}


