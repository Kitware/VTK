## Web Module update

VTK relies on wslink to communicate over WebSocket when building Web applications. VTK now depends on WsLink>=1.0.0 which only depends on asyncio/aiohttp which allow usage of async/await in your python code while reducing the dependency tree.

Now by removing `twisted` we are breaking our backward compatibility with old wslink version.

(follow up of upgrade-wslink)
