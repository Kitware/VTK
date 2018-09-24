namespace diy
{
    struct Master::BaseCommand
    {
        virtual       ~BaseCommand()                                                  {}      // to delete derived classes
        virtual void  execute(void* b, const ProxyWithLink& cp) const                 =0;
        virtual bool  skip(int i, const Master& master) const                         =0;
    };

    template<class Block>
    struct Master::Command: public BaseCommand
    {
              Command(Callback<Block> f_, const Skip& s_):
                  f(f_), s(s_)                                                        {}

        void  execute(void* b, const ProxyWithLink& cp) const override                { f(static_cast<Block*>(b), cp); }
        bool  skip(int i, const Master& m) const override                             { return s(i,m); }

        Callback<Block>   f;
        Skip              s;
    };
}
