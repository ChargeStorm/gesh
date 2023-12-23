#include "dbus-abstraction.h"
#include "setting.h"
#include "settingInitializerHardcoded.h"
#include <sdbus-c++/sdbus-c++.h>
#include "log.h"
#include "version.h"
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>

constexpr auto DBUS_SERVICE = "owl.gesh";
constexpr auto DBUS_PATH = "/owl/gesh";

void signalHandler( int signum ) {
   ERROR << "Interrupt signal (" << signum << ") received.\n";

   // cleanup and close up stuff here
   // terminate program

   exit(signum);
}

class ManagerAdaptor : public sdbus::AdaptorInterfaces< sdbus::ObjectManager_adaptor >
{
public:
    ManagerAdaptor(sdbus::IConnection& connection, std::string path)
    : AdaptorInterfaces(connection, std::move(path))
    {
        registerAdaptor();
    }

    ~ManagerAdaptor()
    {
        unregisterAdaptor();
    }
};

class DbusAdaptor final : public sdbus::AdaptorInterfaces< owl::gesh::setting_adaptor,
                                                sdbus::ManagedObject_adaptor,
                                                sdbus::Properties_adaptor >
{
public:
    DbusAdaptor(sdbus::IConnection& connection, std::string path)
    : AdaptorInterfaces(connection, std::move(path))
    {
        registerAdaptor();
        emitInterfacesAddedSignal({owl::gesh::setting_adaptor::INTERFACE_NAME});
    }

    ~DbusAdaptor()
    {
        emitInterfacesRemovedSignal({owl::gesh::setting_adaptor::INTERFACE_NAME});
        unregisterAdaptor();
    }

    void UpdateRequiresReset(const std::string& name) override
    {
        return;
    }

    std::vector<sdbus::Variant> Get(const std::vector<std::string>& names) override {
        return std::vector<sdbus::Variant>(sdbus::Variant(123));
    }

    std::map<std::string, sdbus::Variant> GetAll() override {
        return std::map<std::string, sdbus::Variant>();
    }

    void Set(const std::map<std::string, sdbus::Variant>& settings) {
        return;
    }

private:
};

int main()
{
    signal(SIGINT, signalHandler);

    INFO << "Gesh - Good enough setting handler" << std::endl;
    INFO << "Git SHA: " << BUILD_VERSION << std::endl;
    INFO << "Build: " << BUILD_DATE << std::endl;

    auto init = SettingInitializerHardcoded();
    std::vector<ISettingInitializer> extraSource;

    auto handler = SettingHandler(init, extraSource);
    for ( auto const& [key, val] : handler.Settings() ) {
        DEBUG << val << std::endl;
    }

    auto connection = sdbus::createSessionBusConnection();
    connection->requestName(DBUS_SERVICE);
    connection->enterEventLoopAsync();

    auto manager = std::make_unique<ManagerAdaptor>(*connection, DBUS_PATH);
    auto settingManager = std::make_unique<DbusAdaptor>(*connection, DBUS_PATH);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    connection->releaseName(DBUS_SERVICE);
    connection->leaveEventLoop();
    return 0;
}
