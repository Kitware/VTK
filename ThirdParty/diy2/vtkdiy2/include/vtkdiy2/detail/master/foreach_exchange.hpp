struct diy::Master::CoroutineArg
{
    int                                                 lid;
    ProxyWithLink&                                      proxy;
    coroutine::cothread_t                               main;
    Master*                                             master;
    std::function<void(int, const ProxyWithLink&)>      f;
};

void
diy::Master::
launch_process_block_coroutine()
{
    using namespace coroutine;

    CoroutineArg*           arg     = static_cast<CoroutineArg*>(argument());
    int                     lid     = arg->lid;
    ProxyWithLink&          cp      = arg->proxy;
    cothread_t              main    = arg->main;
    // unused
    //Master*                 master  = arg->master;
    auto                    f       = arg->f;

    co_switch(main);        // give the argument back

    f(lid, cp);
    cp.set_done(true);
    co_switch(main);
}

template<class F>
void
diy::Master::
foreach_exchange(const F& f, bool remote, unsigned int stack_size)
{
    using Block = typename detail::block_traits<F>::type;
    foreach_exchange_<Block>(f, remote, stack_size);
}

template<class Block>
void
diy::Master::
foreach_exchange_(const CoroutineCallback<Block>& f, bool remote, unsigned int stack_size)
{
    auto scoped = prof.scoped("foreach_exchange");
    DIY_UNUSED(scoped);

    assert(commands_.empty());  // can't have queued, unexecuted commands left over from ordinary foreach()

    using namespace coroutine;

    // setup coroutines and proxies
    std::vector<cothread_t>                     coroutines;
    std::vector<std::unique_ptr<ProxyWithLink>> proxies;
    std::vector<Block*>                         blocks(size(), nullptr);
    for (int i = 0; i < size(); ++i)
    {
        cothread_t c = co_create(stack_size, &launch_process_block_coroutine);
        coroutines.push_back(c);
        proxies.emplace_back(make_unique<ProxyWithLink>(proxy(i)));

        auto trampoline = [&f,&blocks,this](int lid, const ProxyWithLink& cp)
        {
            Block* const& b = blocks[lid];
            f(b, cp);
        };
        CoroutineArg arg { i, *proxies.back(), co_active(), this, trampoline };
        argument() = &arg;

        co_switch(c);
    }

    int ndone = 0;
    std::vector<bool>   done(size(), false);
    while(ndone < size())
    {
        // TODO: parallelize the loop using multiple threads
        for(int i = 0; i < size(); ++i)
        {
            if (done[i] == true)
                continue;

            blocks[i] = get<Block>(i); // load block in case it's out of core

            cothread_t  c  = coroutines[i];
            auto&       cp = *proxies[i];
            cp.init();      // reset incoming/outgoing/collectives
            cp.set_main(co_active());
            co_switch(c);

            if (cp.done())
            {
                done[i] = true;
                ndone++;
            }
        }

        // TODO: should we disable the last exchange; the user could call that one manually
        // exchange calls execute(), but commands_ are empty, so it should be Ok
        exchange(remote);
    }

    for (cothread_t cor : coroutines)
        co_delete(cor);
}
