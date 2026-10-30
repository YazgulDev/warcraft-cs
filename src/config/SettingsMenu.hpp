#pragma once
#include "GameplaySettings.hpp"
#include "ConfigChange.hpp"
#include <array>
#include <functional>
#include <vector>

// A small draft and bounded input mailbox keep UI editing separate from the live gameplay snapshot.
class SettingsMenu {
public:
    static constexpr int Rows=8;
    enum Row {Sensitivity,ScopeSensitivity,Volume,Physics,ViewDistance,SkyVisible,SkyMode,SkyPreset};
    enum class Kind {Toggle,Cancel,Apply,Defaults,Select,Navigate,Adjust,Slide};
    struct Event {Kind kind;int row=-1;float value=0;};
    using SkyList=std::function<std::vector<std::string>()>;
    using Save=std::function<bool(const std::vector<ConfigChange>&)>;
    void Queue(Event event);
    void Reset(){open_=false;events_.clear();error_.clear();}
    bool Open() const {return open_;}
    bool RoutingOpen() const;
    // Successful saves return true so the application can reuse the existing validated F8 apply path.
    bool Update(const GameplaySettings& settings,char tileset,const SkyList& list,const Save& save);
    int Selected() const {return selected_;}
    bool Dirty() const {return values_!=initial_;}
    const std::string& Error() const {return error_;}
    static const char* Label(int row);
    static const char* Hint(int row);
    static bool Numeric(int row){return row==Sensitivity || row==ScopeSensitivity || row==Volume || row==ViewDistance;}
    float Fraction(int row) const;
    std::string Value(int row) const;
private:
    void Begin(const GameplaySettings& settings,char tileset,const SkyList& list);
    void Adjust(int row,float steps);
    std::vector<ConfigChange> Changes() const;
    bool open_=false;int selected_=0;char tileset_=0;
    std::array<float,Rows> values_{},initial_{};
    std::vector<std::string> skies_;
    bool csAvailable_=true;
    std::string savedSky_;
    std::vector<Event> events_;
    std::string error_;
};
