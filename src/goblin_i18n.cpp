#include "goblin_i18n.hpp"
#include "goblin_config.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace
{
    using goblin::i18n::Language;
    using goblin::i18n::TextId;
    using goblin::i18n::ToastId;

    struct TextRow
    {
        TextId id;
        const char *en;
        const char *sc;
        const char *tc;
    };

    struct ToastRow
    {
        ToastId id;
        const wchar_t *en;
        const wchar_t *sc;
        const wchar_t *tc;
    };

    struct NameRow
    {
        const char *key;
        const char *en;
        const char *sc;
        const char *tc;
    };

    std::string lower(std::string_view s)
    {
        std::string out(s);
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }

    const char *pick(Language language, const char *en, const char *sc, const char *tc)
    {
        switch (language)
        {
        case Language::SimplifiedChinese: return (sc && sc[0]) ? sc : en;
        case Language::TraditionalChinese: return (tc && tc[0]) ? tc : en;
        default: return en;
        }
    }

    const wchar_t *wpick(Language language, const wchar_t *en, const wchar_t *sc, const wchar_t *tc)
    {
        switch (language)
        {
        case Language::SimplifiedChinese: return (sc && sc[0]) ? sc : en;
        case Language::TraditionalChinese: return (tc && tc[0]) ? tc : en;
        default: return en;
        }
    }

    const TextRow TEXTS[] = {
        {TextId::IniHeader,
         "MapForGoblins configuration. Auto-generated from the in-code schema;\nthe DLL re-syncs this file on launch (adds new keys, comments out removed ones).",
         "MapForGoblins 配置。此文件由代码内置结构自动生成；\nDLL 启动时会重新同步本文件（添加新键，并注释掉已移除的键）。",
         "MapForGoblins 設定。此檔案由程式碼內建結構自動產生；\nDLL 啟動時會重新同步此檔案（新增按鍵，並註解掉已移除的按鍵）。"},
        {TextId::AllOn, "all on", "全开", "全開"},
        {TextId::AllOff, "all off", "全关", "全關"},
        {TextId::RandomizerHint,
         "Using a Randomizer? Enable live_loot_flags + live_loot_labels + live_loot_icons so markers match the shuffled item placements.",
         "使用随机道具？请启用 live_loot_flags + live_loot_labels + live_loot_icons，让标记匹配被打乱后的物品位置。",
         "使用隨機道具？請啟用 live_loot_flags + live_loot_labels + live_loot_icons，讓標記符合被打亂後的物品位置。"},
        {TextId::FastMapOpenWarning,
         "BETA: makes the map open faster. If the map glitches or crashes, turn this off.",
         "测试功能：加快地图打开速度。如果地图异常或崩溃，请关闭它。",
         "測試功能：加快地圖開啟速度。如果地圖異常或崩潰，請關閉它。"},
        {TextId::IniOnly, "(.ini only)", "（仅限 INI）", "（僅限 INI）"},
        {TextId::PressAKey, "press a key", "按一个按键", "按一個按鍵"},
        {TextId::PressComboRelease, "press combo, release", "按住组合后松开", "按住組合後放開"},
        {TextId::ReopenMapWarning,
         "You MUST re-open the world map to see changes!",
         "必须重新打开世界地图才能看到变化！",
         "必須重新開啟世界地圖才能看到變更！"},
        {TextId::AllIconCategories, "All icon categories:", "所有图标类别：", "所有圖示類別："},
        {TextId::ShowAll, "show all", "全部显示", "全部顯示"},
        {TextId::HideAll, "hide all", "全部隐藏", "全部隱藏"},
        {TextId::DebugDumpDescription,
         "Dumps the map beacons (your 1-5 placed markers) and stamps to text. Works whether the map is open or closed. Press Copy to put it on the clipboard.",
         "把地图信标（你放置的 1-5 号标记）和印记导出为文本。地图打开或关闭时都可用。按“复制”可写入剪贴板。",
         "把地圖信標（你放置的 1-5 號標記）和印記匯出為文字。地圖開啟或關閉時都可用。按「複製」可寫入剪貼簿。"},
        {TextId::DumpMarkersNow, "Dump markers now", "立即导出标记", "立即匯出標記"},
        {TextId::Copy, "Copy", "复制", "複製"},
        {TextId::Chars, "chars", "个字符", "個字元"},
        {TextId::NoDumpYet, "(no dump yet - press the button above)", "（尚未导出，请按上面的按钮）", "（尚未匯出，請按上方按鈕）"},
        {TextId::AboutDescription,
         "Thousands of loot and world icons on the in-game map.",
         "在游戏内地图上显示数千个战利品与世界图标。",
         "在遊戲內地圖上顯示數千個戰利品與世界圖示。"},
        {TextId::Version, "Version", "版本", "版本"},
        {TextId::LinkNexus, "Nexus Mods", "Nexus Mods", "Nexus Mods"},
        {TextId::LinkGithub, "GitHub (source)", "GitHub（源码）", "GitHub（原始碼）"},
        {TextId::LinkDiscord, "Discord (support)", "Discord（支持）", "Discord（支援）"},
        {TextId::ControlHintGamepad,
         "D-Pad/Stick: move   A: toggle   B: close   LB/RB: switch tab",
         "方向键/摇杆：移动   A：切换   B：关闭   LB/RB：切换标签",
         "方向鍵/搖桿：移動   A：切換   B：關閉   LB/RB：切換分頁"},
        {TextId::ControlHintKeyboard,
         "Mouse + Arrows: move   Click/Space: toggle   Esc: close",
         "鼠标 + 方向键：移动   点击/空格：切换   Esc：关闭",
         "滑鼠 + 方向鍵：移動   點擊/空白鍵：切換   Esc：關閉"},
        {TextId::WindowTitle, "Map for Goblins - Settings", "Map for Goblins - 设置", "Map for Goblins - 設定"},
        {TextId::MasterToggle, "Show map icons (master)", "显示地图图标（总开关）", "顯示地圖圖示（總開關）"},
        {TextId::MasterToggleTooltip,
         "Turns ALL map icons on/off at once.\nWhen the overlay is disabled, toggle_key / the gamepad combo do this from outside the menu.",
         "一次性开启/关闭所有地图图标。\n当菜单被禁用时，toggle_key / 手柄组合键会在菜单外执行这个切换。",
         "一次開啟/關閉所有地圖圖示。\n當選單被停用時，toggle_key / 手把組合鍵會在選單外執行此切換。"},
        {TextId::Close, "Close", "关闭", "關閉"},
        {TextId::TabSettings, "Settings", "设置", "設定"},
        {TextId::TabDebug, "Debug", "调试", "偵錯"},
        {TextId::TabAbout, "About", "关于", "關於"},
    };

    const ToastRow TOASTS[] = {
        {ToastId::MapIconsOn, L"Map icons: ON", L"地图图标：开启", L"地圖圖示：開啟"},
        {ToastId::MapIconsOff, L"Map icons: OFF", L"地图图标：关闭", L"地圖圖示：關閉"},
        {ToastId::MarkersDumped, L"Markers dumped", L"标记已导出", L"標記已匯出"},
        {ToastId::MarkerDumpFailed, L"Marker dump failed - press again", L"标记导出失败，请再按一次", L"標記匯出失敗，請再按一次"},
    };

    const NameRow SECTION_LABELS[] = {
        {"Goblin", "Goblin", "基础", "基礎"},
        {"Equipment", "Equipment", "装备", "裝備"},
        {"Key Items", "Key Items", "关键道具", "關鍵道具"},
        {"Loot", "Loot", "战利品", "戰利品"},
        {"Magic", "Magic", "魔法/祷告", "魔法/禱告"},
        {"Quest", "Quest", "任务", "任務"},
        {"Reforged", "Reforged", "Reforged", "Reforged"},
        {"World", "World", "世界", "世界"},
        {"ERR Markers", "ERR Markers", "ERR 原生标记", "ERR 原生標記"},
        {"Compatibility", "Compatibility", "兼容性", "相容性"},
        {"Overlay & Hotkeys", "Overlay & Hotkeys", "菜单与热键", "選單與快捷鍵"},
        {"Debug", "Debug", "调试", "偵錯"},
    };

    const NameRow SECTION_COMMENTS[] = {
        {"Reforged",
         "Elden Ring Reforged-only content. Absent from the vanilla build.",
         "仅 Elden Ring Reforged 内容。原版构建中不会出现。",
         "僅 Elden Ring Reforged 內容。原版建置中不會出現。"},
        {"ERR Markers",
         "This section applies this mod's display rules to ERR's OWN pre-placed map\nmarkers (camps, merchants, bosses, dungeon entrances) - NOT the icons this\nmod injects - so both icon sets follow the same visibility logic\n(map-fragment discovery, hide on clear). Disable a toggle to leave that\nmarker group exactly as ERR ships it. Our own boss markers are\n[World] show_bosses and independent of this section.",
         "本节会把本 MOD 的显示规则应用到 ERR 自带的预设地图标记\n（营地、商人、Boss、地下城入口）上，而不是本 MOD 注入的图标。\n这样两套图标会遵循相同的可见性逻辑（地图碎片发现、清除后隐藏）。\n关闭某个开关即可让对应标记组保持 ERR 原样。本 MOD 自己的 Boss 标记\n由 [World] show_bosses 控制，和本节相互独立。",
         "本節會把本 MOD 的顯示規則套用到 ERR 自帶的預設地圖標記\n（營地、商人、Boss、地下城入口）上，而不是本 MOD 注入的圖示。\n如此兩套圖示會遵循相同的可見性邏輯（地圖碎片發現、清除後隱藏）。\n關閉某個開關即可讓對應標記組保持 ERR 原樣。本 MOD 自己的 Boss 標記\n由 [World] show_bosses 控制，和本節相互獨立。"},
        {"Compatibility",
         "Options for running alongside other mods that change item placement.",
         "与改变物品摆放的其他 MOD 共用时的选项。",
         "與改變物品擺放的其他 MOD 共用時的選項。"},
        {"Overlay & Hotkeys",
         "The in-game config overlay and the key/button that opens it. toggle_key\n(keyboard) / toggle_gamepad_combo (gamepad) OPEN the overlay when enable_overlay\nis on, or toggle ALL map icons on/off when it's off. Key names: F1-F24, A-Z,\n0-9, Space, Escape, Tab, Enter, Backspace, Home, End, PageUp, PageDown, Insert,\nDelete, arrows.",
         "游戏内设置菜单，以及打开菜单的键盘/手柄按键。enable_overlay 开启时，\ntoggle_key（键盘）/ toggle_gamepad_combo（手柄）会打开菜单；关闭时则会\n开关所有地图图标。按键名支持：F1-F24、A-Z、0-9、Space、Escape、Tab、\nEnter、Backspace、Home、End、PageUp、PageDown、Insert、Delete、方向键。",
         "遊戲內設定選單，以及開啟選單的鍵盤/手把按鍵。enable_overlay 開啟時，\ntoggle_key（鍵盤）/ toggle_gamepad_combo（手把）會開啟選單；關閉時則會\n開關所有地圖圖示。按鍵名稱支援：F1-F24、A-Z、0-9、Space、Escape、Tab、\nEnter、Backspace、Home、End、PageUp、PageDown、Insert、Delete、方向鍵。"},
        {"Debug", "Diagnostics, shown on the overlay's Debug tab.", "诊断选项，会显示在菜单的“调试”标签页。", "診斷選項，會顯示在選單的「偵錯」分頁。"},
    };

    const NameRow ENTRY_LABELS[] = {
        {"load_delay", "load_delay", "加载延迟", "載入延遲"},
        {"require_map_fragments", "require_map_fragments", "需要地图碎片", "需要地圖碎片"},
        {"fast_map_open", "fast_map_open", "加快地图打开", "加快地圖開啟"},
        {"show_armaments", "show_armaments", "武器", "武器"},
        {"show_armour", "show_armour", "防具", "防具"},
        {"show_ashes_of_war", "show_ashes_of_war", "战灰", "戰灰"},
        {"show_spirits", "show_spirits", "骨灰", "骨灰"},
        {"show_talismans", "show_talismans", "护符", "護符"},
        {"show_celestial_dew", "show_celestial_dew", "星星泪滴", "星星淚滴"},
        {"show_cookbooks", "show_cookbooks", "制作笔记", "製作筆記"},
        {"show_crystal_tears", "show_crystal_tears", "结晶露滴", "結晶露滴"},
        {"show_great_runes", "show_great_runes", "大卢恩", "大盧恩"},
        {"show_imbued_sword_keys", "show_imbued_sword_keys", "魔石剑钥匙", "魔石劍鑰匙"},
        {"show_larval_tears", "show_larval_tears", "泪滴幼体", "淚滴幼體"},
        {"show_lost_ashes", "show_lost_ashes", "失力战灰", "失力戰灰"},
        {"show_pots_n_perfumes", "show_pots_n_perfumes", "壶与调香瓶", "壺與調香瓶"},
        {"show_scadutree_fragments", "show_scadutree_fragments", "幽影树碎片", "幽影樹碎片"},
        {"show_seeds_tears", "show_seeds_tears", "种子/露滴/灵灰", "種子/露滴/靈灰"},
        {"show_whetblades", "show_whetblades", "砥石刀", "砥石刀"},
        {"show_ammo", "show_ammo", "箭矢/弩箭", "箭矢/弩箭"},
        {"show_bell_bearings", "show_bell_bearings", "铃珠", "鈴珠"},
        {"show_merchant_bell_bearings", "show_merchant_bell_bearings", "商人铃珠", "商人鈴珠"},
        {"show_consumables", "show_consumables", "消耗品", "消耗品"},
        {"show_greases", "show_greases", "油脂", "油脂"},
        {"show_utilities", "show_utilities", "实用道具", "實用道具"},
        {"show_stat_boosts", "show_stat_boosts", "属性强化道具", "能力強化道具"},
        {"show_crafting_materials", "show_crafting_materials", "制作材料", "製作材料"},
        {"show_gloveworts", "show_gloveworts", "墓地/灵依铃兰", "墓地/靈依鈴蘭"},
        {"show_golden_runes", "show_golden_runes", "卢恩道具", "盧恩道具"},
        {"show_golden_runes_low", "show_golden_runes_low", "低级卢恩道具", "低階盧恩道具"},
        {"show_great_gloveworts", "show_great_gloveworts", "大朵铃兰", "大朵鈴蘭"},
        {"show_material_nodes", "show_material_nodes", "一次性采集点", "一次性採集點"},
        {"show_mp_fingers", "show_mp_fingers", "多人联机道具", "多人連線道具"},
        {"show_prattling_pates", "show_prattling_pates", "唤声泥颅", "喚聲泥顱"},
        {"show_rada_fruit", "show_rada_fruit", "拉达果实", "拉達果實"},
        {"show_gestures", "show_gestures", "肢体动作", "肢體動作"},
        {"show_reusables", "show_reusables", "可重复使用道具", "可重複使用道具"},
        {"show_smithing_stones", "show_smithing_stones", "锻造石", "鍛造石"},
        {"show_smithing_stones_low", "show_smithing_stones_low", "低级锻造石", "低階鍛造石"},
        {"show_smithing_stones_rare", "show_smithing_stones_rare", "古龙锻造石", "古龍鍛造石"},
        {"show_stonesword_keys", "show_stonesword_keys", "石剑钥匙", "石劍鑰匙"},
        {"show_throwables", "show_throwables", "投掷道具", "投擲道具"},
        {"show_rune_arcs", "show_rune_arcs", "卢恩弯弧", "盧恩彎弧"},
        {"show_dragon_hearts", "show_dragon_hearts", "龙心脏", "龍心臟"},
        {"show_incantations", "show_incantations", "祷告", "禱告"},
        {"show_memory_stones", "show_memory_stones", "记忆石", "記憶石"},
        {"show_prayerbooks", "show_prayerbooks", "祷告书/卷轴", "禱告書/卷軸"},
        {"show_sorceries", "show_sorceries", "魔法", "魔法"},
        {"show_deathroot", "show_deathroot", "死根", "死根"},
        {"show_progression", "show_progression", "任务进度道具", "任務進度道具"},
        {"show_seedbed_curses", "show_seedbed_curses", "温床的诅咒", "溫床的詛咒"},
        {"show_ember_pieces", "show_ember_pieces", "ERR 余火碎片", "ERR 餘火碎片"},
        {"show_items_and_changes", "show_items_and_changes", "ERR 新增道具", "ERR 新增道具"},
        {"show_fortunes", "show_fortunes", "ERR Fortune 饰品", "ERR Fortune 飾品"},
        {"show_rune_pieces", "show_rune_pieces", "ERR 卢恩碎片", "ERR 盧恩碎片"},
        {"show_bosses", "show_bosses", "Boss 标记", "Boss 標記"},
        {"show_graces", "show_graces", "赐福点", "賜福點"},
        {"show_hostile_npc", "show_hostile_npc", "敌对 NPC", "敵對 NPC"},
        {"show_imp_statues", "show_imp_statues", "小恶魔雕像", "小惡魔雕像"},
        {"show_paintings", "show_paintings", "绘画", "繪畫"},
        {"show_spirit_springs", "show_spirit_springs", "灵魂气流", "靈魂氣流"},
        {"show_spiritspring_hawks", "show_spiritspring_hawks", "灵魂气流鹰", "靈魂氣流鷹"},
        {"show_stakes_of_marika", "show_stakes_of_marika", "玛莉卡楔石", "瑪莉卡楔石"},
        {"show_summoning_pools", "show_summoning_pools", "召唤池", "召喚池"},
        {"show_kindling_spirits", "show_kindling_spirits", "Kindling Spirits", "Kindling Spirits"},
        {"show_interactables", "show_interactables", "互动世界物件", "互動世界物件"},
        {"show_world_maps", "show_world_maps", "地图碎片", "地圖碎片"},
        {"hide_killed_bosses", "hide_killed_bosses", "击败后隐藏 Boss", "擊敗後隱藏 Boss"},
        {"patch_overworld_boss_icons", "patch_overworld_boss_icons", "修复野外 Boss 图标", "修復野外 Boss 圖示"},
        {"patch_dungeon_boss_icons", "patch_dungeon_boss_icons", "修复地下城入口图标", "修復地下城入口圖示"},
        {"patch_camp_icons", "patch_camp_icons", "修复营地图标", "修復營地圖示"},
        {"patch_merchant_icons", "patch_merchant_icons", "修复商人图标", "修復商人圖示"},
        {"hide_dungeon_icons_on_clear", "hide_dungeon_icons_on_clear", "通关后隐藏地下城入口", "通關後隱藏地下城入口"},
        {"live_loot_flags", "live_loot_flags", "使用实时拾取旗标", "使用即時拾取旗標"},
        {"live_loot_labels", "live_loot_labels", "使用实时战利品名称", "使用即時戰利品名稱"},
        {"live_loot_icons", "live_loot_icons", "使用实时战利品图标", "使用即時戰利品圖示"},
        {"anonymous_loot", "anonymous_loot", "匿名战利品模式", "匿名戰利品模式"},
        {"rebind", "rebind", "重绑", "重新綁定"},
        {"ui_language", "ui_language", "菜单语言", "選單語言"},
        {"enable_overlay", "enable_overlay", "启用游戏内菜单", "啟用遊戲內選單"},
        {"enable_toggle_hotkey", "enable_toggle_hotkey", "启用热键", "啟用快捷鍵"},
        {"toggle_key", "toggle_key", "菜单热键", "選單快捷鍵"},
        {"toggle_gamepad_combo", "toggle_gamepad_combo", "手柄组合键", "手把組合鍵"},
        {"debug_logging", "debug_logging", "调试日志", "偵錯記錄"},
        {"enable_marker_dump", "enable_marker_dump", "启用标记导出", "啟用標記匯出"},
        {"marker_dump_key", "marker_dump_key", "标记导出热键", "標記匯出快捷鍵"},
    };

    const NameRow ENTRY_COMMENTS[] = {
        {"load_delay", "Delay in seconds before loading map icons (wait for game initialization)", "加载地图图标前等待的秒数（等待游戏初始化）。", "載入地圖圖示前等待的秒數（等待遊戲初始化）。"},
        {"require_map_fragments", "Require map fragment discovery before showing icons in that area", "需要先发现对应区域的地图碎片，才显示该区域图标。", "需要先發現對應區域的地圖碎片，才顯示該區域圖示。"},
        {"fast_map_open", "BETA: makes the world map open faster when many icons are shown.\nTurn off if the map glitches or crashes.", "测试功能：显示大量图标时加快世界地图打开速度。\n如果地图异常或崩溃，请关闭。", "測試功能：顯示大量圖示時加快世界地圖開啟速度。\n如果地圖異常或崩潰，請關閉。"},
        {"show_armaments", "Weapons, shields, bows, staves, etc.", "武器、盾牌、弓、杖等。", "武器、盾牌、弓、杖等。"},
        {"show_armour", "Armor pieces (helms, chest, gauntlets, legs)", "防具部位（头盔、胸甲、臂甲、腿甲）。", "防具部位（頭盔、胸甲、臂甲、腿甲）。"},
        {"show_ashes_of_war", "Ashes of War (weapon skills)", "战灰（武器战技）。", "戰灰（武器戰技）。"},
        {"show_spirits", "Spirit Ashes (summons)", "骨灰（召唤灵）。", "骨灰（召喚靈）。"},
        {"show_talismans", "Talismans", "护符。", "護符。"},
        {"show_celestial_dew", "Celestial Dew (for reversing Spirit Ashes upgrades)", "星星泪滴（ERR 中用于重置骨灰强化）。", "星星淚滴（ERR 中用於重置骨灰強化）。"},
        {"show_cookbooks", "Cookbooks (crafting recipes)", "制作笔记（解锁制作配方）。", "製作筆記（解鎖製作配方）。"},
        {"show_crystal_tears", "Crystal Tears (for Flask of Wondrous Physick)", "结晶露滴（灵药圣杯瓶）。", "結晶露滴（靈藥聖杯瓶）。"},
        {"show_great_runes", "Great Runes (dropped by story bosses)", "大卢恩（主线 Boss 掉落）。", "大盧恩（主線 Boss 掉落）。"},
        {"show_imbued_sword_keys", "Imbued Sword Keys (Four Belfries)", "魔石剑钥匙（四钟楼）。", "魔石劍鑰匙（四鐘樓）。"},
        {"show_larval_tears", "Larval Tears (respec items)", "泪滴幼体（洗点道具）。", "淚滴幼體（重生道具）。"},
        {"show_lost_ashes", "Lost Ashes of War", "失力战灰。", "失力戰灰。"},
        {"show_pots_n_perfumes", "Cracked Pots, Ritual Pots, Perfume Bottles", "龟裂壶、仪式壶、调香瓶。", "龜裂壺、儀式壺、調香瓶。"},
        {"show_scadutree_fragments", "Scadutree Fragments (DLC blessing upgrade)", "幽影树碎片（DLC 祝福强化）。", "幽影樹碎片（DLC 祝福強化）。"},
        {"show_seeds_tears", "Golden Seeds, Sacred Tears, Revered Spirit Ashes", "黄金种子、圣杯露滴、灵灰。", "黃金種子、聖杯露滴、靈灰。"},
        {"show_whetblades", "Whetblades (weapon infusion types)", "砥石刀（武器质变类型）。", "砥石刀（武器質變類型）。"},
        {"show_ammo", "Arrows, bolts, greatarrows, greatbolts", "箭、弩箭、大箭、大弩箭。", "箭、弩箭、大箭、大弩箭。"},
        {"show_bell_bearings", "Bell Bearings from treasures/chests/quest rewards", "宝箱、宝物和任务奖励中的铃珠。", "寶箱、寶物和任務獎勵中的鈴珠。"},
        {"show_merchant_bell_bearings", "Bell Bearings dropped by killing merchants (Kale, Patches, Gostoc,\nnomadic merchants, etc.)", "杀死商人后掉落的铃珠（咖列、帕奇、葛托克、游牧商人等）。", "殺死商人後掉落的鈴珠（咖列、帕奇、葛托克、流浪商人等）。"},
        {"show_consumables", "Healing/buff consumables (boluses, cured meats, livers)", "治疗/增益类消耗品（苔药、腌肉、肝脏等）。", "治療/增益類消耗品（苔藥、醃肉、肝臟等）。"},
        {"show_greases", "Weapon greases", "武器油脂。", "武器油脂。"},
        {"show_utilities", "Utility items (rainbow stone, glowstone, soap, soft cotton)", "实用道具（七色石、发光石、肥皂、柔软棉花等）。", "實用道具（七色石、發光石、肥皂、柔軟棉花等）。"},
        {"show_stat_boosts", "Stat-up items (Starlight Shards, Sacrificial Twig, Blessing of Marika)", "属性/资源强化道具（星光碎片、牺牲细枝、玛莉卡的赐福等）。", "能力/資源強化道具（星光碎片、犧牲細枝、瑪莉卡的賜福等）。"},
        {"show_crafting_materials", "Crafting materials (flowers, bones, bugs, etc.)", "制作材料（花、骨头、虫等）。", "製作材料（花、骨頭、蟲等）。"},
        {"show_gloveworts", "Gloveworts (Grave/Ghost [1-9]) - Spirit Ash upgrade materials", "墓地/灵依铃兰 [1-9]，骨灰强化材料。", "墓地/靈依鈴蘭 [1-9]，骨灰強化材料。"},
        {"show_golden_runes", "Golden Runes [4000+], Hero's/Numen's/Lord's/Shadow Realm Runes", "黄金卢恩 [4000+]、英雄/稀人/王之卢恩、幽影之地卢恩。", "黃金盧恩 [4000+]、英雄/稀人/王之盧恩、幽影之地盧恩。"},
        {"show_golden_runes_low", "Golden Runes [200-3000], Broken Runes", "黄金卢恩 [200-3000]、碎裂卢恩。", "黃金盧恩 [200-3000]、碎裂盧恩。"},
        {"show_great_gloveworts", "Great Gloveworts (Great Grave, Great Ghost)", "大朵墓地铃兰与大朵灵依铃兰。", "大朵墓地鈴蘭與大朵靈依鈴蘭。"},
        {"show_material_nodes", "One-time gathering nodes (Erdleaf Flower, Trina's Lily, etc.)", "一次性采集点（落叶花、托莉娜睡莲等）。", "一次性採集點（落葉花、托莉娜睡蓮等）。"},
        {"show_mp_fingers", "Multiplayer items (Furlcalling/Wizened Fingers, Recusant/Bloody Finger)", "多人联机道具（唤勾指药、枯瘦手指、叛律/血指等）。", "多人連線道具（喚勾指藥、枯瘦手指、叛律/血指等）。"},
        {"show_prattling_pates", "Prattling Pates", "唤声泥颅。", "喚聲泥顱。"},
        {"show_rada_fruit", "Rada Fruit (DLC stat-up consumable)", "拉达果实（ERR DLC 属性强化消耗品）。", "拉達果實（ERR DLC 能力強化消耗品）。"},
        {"show_gestures", "Gestures", "肢体动作。", "肢體動作。"},
        {"show_reusables", "Reusable tools (Mimic Veil, Margit's Shackle, etc.)", "可重复使用工具（拟态面纱、恶兆妖鬼的囚具等）。", "可重複使用工具（擬態面紗、惡兆妖鬼的囚具等）。"},
        {"show_smithing_stones", "Smithing Stones [7-8], Somber [7-9], Scadushards", "锻造石 [7-8]、失色锻造石 [7-9]、幽影锻造碎片。", "鍛造石 [7-8]、失色鍛造石 [7-9]、幽影鍛造碎片。"},
        {"show_smithing_stones_low", "Smithing Stones [1-6], Somber [1-6]", "锻造石 [1-6]、失色锻造石 [1-6]。", "鍛造石 [1-6]、失色鍛造石 [1-6]。"},
        {"show_smithing_stones_rare", "Ancient Dragon Smithing Stones (rare, endgame)", "古龙锻造石（稀有，后期）。", "古龍鍛造石（稀有，後期）。"},
        {"show_stonesword_keys", "Stonesword Keys", "石剑钥匙。", "石劍鑰匙。"},
        {"show_throwables", "Throwable items (darts, daggers, stones, chakrams, warming stones)", "投掷道具（飞镖、飞刀、石头、环刃、温热石等）。", "投擲道具（飛鏢、飛刀、石頭、環刃、溫熱石等）。"},
        {"show_rune_arcs", "Rune Arcs (buffs for active Great Rune)", "卢恩弯弧（激活大卢恩效果）。", "盧恩彎弧（啟用大盧恩效果）。"},
        {"show_dragon_hearts", "Dragon Hearts (for Dragon Communion incantations)", "龙心脏（龙飨祷告）。", "龍心臟（龍饗禱告）。"},
        {"show_incantations", "Incantation locations", "祷告位置。", "禱告位置。"},
        {"show_memory_stones", "Memory Stone locations (extra spell slots)", "记忆石位置（增加法术记忆栏）。", "記憶石位置（增加法術記憶欄）。"},
        {"show_prayerbooks", "Prayerbooks and Scrolls (unlock spells at vendors)", "祷告书与卷轴（在商人处解锁法术）。", "禱告書與卷軸（在商人處解鎖法術）。"},
        {"show_sorceries", "Sorcery locations", "魔法位置。", "魔法位置。"},
        {"show_deathroot", "Deathroot locations (for Gurranq)", "死根位置（交给古兰格）。", "死根位置（交給古蘭格）。"},
        {"show_progression", "Quest progression items (medallions, keys, Needles, quest-specific goods)", "任务推进道具（符节、钥匙、针、任务专用道具等）。", "任務推進道具（符節、鑰匙、針、任務專用道具等）。"},
        {"show_seedbed_curses", "Seedbed Curse locations (for Dung Eater quest)", "温床的诅咒位置（食粪者任务）。", "溫床的詛咒位置（食糞者任務）。"},
        {"show_ember_pieces", "ERR Ember Piece locations", "ERR 余火碎片位置。", "ERR 餘火碎片位置。"},
        {"show_items_and_changes", "ERR-added items: Oracle Effigy/Remedy, Starlight Tokens, Sealed Curios", "ERR 新增道具：Oracle Effigy/Remedy、Starlight Tokens、Sealed Curios。", "ERR 新增道具：Oracle Effigy/Remedy、Starlight Tokens、Sealed Curios。"},
        {"show_fortunes", "ERR Fortune trinkets (12 types)", "ERR Fortune 饰品（12 种）。", "ERR Fortune 飾品（12 種）。"},
        {"show_rune_pieces", "ERR Rune Piece locations", "ERR 卢恩碎片位置。", "ERR 盧恩碎片位置。"},
        {"show_bosses", "Boss markers (field bosses, dungeon bosses)", "Boss 标记（野外 Boss、地下城 Boss）。", "Boss 標記（野外 Boss、地下城 Boss）。"},
        {"show_graces", "Sites of Grace", "赐福点。", "賜福點。"},
        {"show_hostile_npc", "Hostile NPC invader locations", "敌对 NPC 入侵位置。", "敵對 NPC 入侵位置。"},
        {"show_imp_statues", "Imp Statue (Stonesword Key fog gate) locations", "小恶魔雕像（石剑钥匙雾门）位置。", "小惡魔雕像（石劍鑰匙霧門）位置。"},
        {"show_paintings", "Painting locations", "绘画位置。", "繪畫位置。"},
        {"show_spirit_springs", "Spirit Spring (horse jump) locations", "灵魂气流（灵马跳跃）位置。", "靈魂氣流（靈馬跳躍）位置。"},
        {"show_spiritspring_hawks", "Spiritspring Hawk locations", "灵魂气流鹰位置。", "靈魂氣流鷹位置。"},
        {"show_stakes_of_marika", "Stakes of Marika (respawn points)", "玛莉卡楔石（重生点）位置。", "瑪莉卡楔石（重生點）位置。"},
        {"show_summoning_pools", "Summoning Pool (Martyr Effigy) locations", "召唤池（殉道者偶像）位置。", "召喚池（殉道者偶像）位置。"},
        {"show_kindling_spirits", "ERR Kindling Spirits in Misty Forest - collect all 5 between rests for\nthe Kindling Spirit incantation. Markers hide once you have the incantation.", "ERR 迷雾森林的 Kindling Spirits。需在休息前收集全部 5 个以获得\nKindling Spirit 祷告。获得祷告后标记会隐藏。", "ERR 迷霧森林的 Kindling Spirits。需在休息前收集全部 5 個以取得\nKindling Spirit 禱告。取得禱告後標記會隱藏。"},
        {"show_interactables", "Interactive world objects & puzzles: blue seal puzzles (unlock hidden\ncellars), light-flame interacts (Sellia chalices, Snow Town statues, Siofra\nRiver lanterns), and Hero's Tomb direction statues.", "互动世界物件与谜题：蓝色封印谜题（解锁隐藏地下室）、点火互动\n（瑟利亚火盆、雪镇雕像、希芙拉河灯柱）以及英雄墓地指路雕像。", "互動世界物件與謎題：藍色封印謎題（解鎖隱藏地下室）、點火互動\n（瑟利亞火盆、雪鎮雕像、希芙拉河燈柱）以及英雄墓地指路雕像。"},
        {"show_world_maps", "World Map fragment locations", "世界地图碎片位置。", "世界地圖碎片位置。"},
        {"hide_killed_bosses", "Hide boss/invader/hawk markers after defeat (false = show green checkmark instead)", "击败后隐藏 Boss/入侵者/鹰标记（false = 显示绿色对勾）。", "擊敗後隱藏 Boss/入侵者/鷹標記（false = 顯示綠色勾選）。"},
        {"patch_overworld_boss_icons", "Apply this mod's map-fragment discovery rule to ERR's overworld\nfield-boss markers.", "把本 MOD 的地图碎片发现规则应用到 ERR 的野外 Boss 标记。", "把本 MOD 的地圖碎片發現規則套用到 ERR 的野外 Boss 標記。"},
        {"patch_dungeon_boss_icons", "Apply this mod's map-fragment discovery rule to ERR's dungeon/cave\nentrance markers. Required for hide_dungeon_icons_on_clear below.", "把本 MOD 的地图碎片发现规则应用到 ERR 的地下城/洞窟入口标记。\n下面的 hide_dungeon_icons_on_clear 需要此项开启。", "把本 MOD 的地圖碎片發現規則套用到 ERR 的地下城/洞窟入口標記。\n下方的 hide_dungeon_icons_on_clear 需要此項開啟。"},
        {"patch_camp_icons", "Apply this mod's map-fragment discovery rule to ERR's enemy camp markers.", "把本 MOD 的地图碎片发现规则应用到 ERR 的敌营标记。", "把本 MOD 的地圖碎片發現規則套用到 ERR 的敵營標記。"},
        {"patch_merchant_icons", "Apply this mod's map-fragment discovery rule to ERR's merchant markers.", "把本 MOD 的地图碎片发现规则应用到 ERR 的商人标记。", "把本 MOD 的地圖碎片發現規則套用到 ERR 的商人標記。"},
        {"hide_dungeon_icons_on_clear", "When patching dungeon entrances, hide the marker once the boss inside is\ndefeated. Requires patch_dungeon_boss_icons.", "修复地下城入口时，在内部 Boss 被击败后隐藏入口标记。\n需要 patch_dungeon_boss_icons。", "修復地下城入口時，在內部 Boss 被擊敗後隱藏入口標記。\n需要 patch_dungeon_boss_icons。"},
        {"live_loot_flags", "Hide loot markers using the pickup flag from the loaded regulation, so they\ndisappear correctly under the Item/Enemy Randomizer or other regulation mods.", "使用已加载 regulation 中的拾取旗标隐藏战利品标记，使其能在\nItem/Enemy Randomizer 或其他 regulation MOD 下正确消失。", "使用已載入 regulation 中的拾取旗標隱藏戰利品標記，使其能在\nItem/Enemy Randomizer 或其他 regulation MOD 下正確消失。"},
        {"live_loot_labels", "Relabel each loot marker with the item its lot currently gives, so names match\nthe randomizer. Uses more memory (copies item names into the map's name table).", "把每个战利品标记改名为当前 lot 实际给予的物品，使名称匹配随机器。\n会使用更多内存（把物品名复制到地图名称表中）。", "把每個戰利品標記改名為目前 lot 實際給予的物品，使名稱符合隨機器。\n會使用更多記憶體（把物品名稱複製到地圖名稱表中）。"},
        {"live_loot_icons", "Give each loot marker the icon and category of the item its lot currently\ngives, so icons and show_* toggles match the randomizer.", "让每个战利品标记使用当前 lot 实际物品的图标和类别，\n使图标和 show_* 开关匹配随机器。", "讓每個戰利品標記使用目前 lot 實際物品的圖示和類別，\n使圖示和 show_* 開關符合隨機器。"},
        {"anonymous_loot", "Spoiler-free: every loot marker shows a gray \"?\" and a generic label instead\nof the real item. Overrides live_loot_labels/icons; markers still hide on pickup.", "无剧透：所有战利品标记显示灰色“?”和通用名称，而不显示真实物品。\n会覆盖 live_loot_labels/icons；标记仍会在拾取后隐藏。", "無劇透：所有戰利品標記顯示灰色「?」和通用名稱，而不顯示真實物品。\n會覆蓋 live_loot_labels/icons；標記仍會在拾取後隱藏。"},
        {"ui_language", "Overlay language. auto follows the Steam game language; unrecognized languages\nfall back to English.", "菜单语言。auto 会跟随 Steam 游戏语言；无法识别时回退到英文。", "選單語言。auto 會跟隨 Steam 遊戲語言；無法辨識時回退到英文。"},
        {"enable_overlay", "In-game config overlay (Dear ImGui) opened with the toggle key below.\nSet false if a DX-hook conflict (Steam overlay/RTSS/GeForce Experience) or a\nGPU driver issue makes the game unstable.", "游戏内设置菜单（Dear ImGui），用下面的热键打开。\n如果 DX Hook 与 Steam 覆盖层/RTSS/GeForce Experience 冲突，或 GPU 驱动导致游戏不稳定，可设为 false。", "遊戲內設定選單（Dear ImGui），用下方快捷鍵開啟。\n如果 DX Hook 與 Steam 覆蓋層/RTSS/GeForce Experience 衝突，或 GPU 驅動導致遊戲不穩，可設為 false。"},
        {"enable_toggle_hotkey", "Enable toggle_key / toggle_gamepad_combo to switch ALL map icons on/off when\nthe overlay is DISABLED. (When the overlay is enabled, they open it instead.)", "当菜单被禁用时，允许 toggle_key / toggle_gamepad_combo 开关所有地图图标。\n菜单启用时，这些按键会打开菜单。", "當選單被停用時，允許 toggle_key / toggle_gamepad_combo 開關所有地圖圖示。\n選單啟用時，這些按鍵會開啟選單。"},
        {"toggle_key", "Keyboard toggle: OPENS the config overlay when enable_overlay is on, or\ntoggles ALL map icons on/off when the overlay is disabled. Default: F10.", "键盘热键：enable_overlay 开启时打开设置菜单；菜单禁用时开关所有地图图标。\n默认：F10。", "鍵盤快捷鍵：enable_overlay 開啟時開啟設定選單；選單停用時開關所有地圖圖示。\n預設：F10。"},
        {"toggle_gamepad_combo", "Gamepad toggle, same role as toggle_key (opens the overlay, or toggles all\nicons if the overlay is disabled). Tokens joined with '+': A,B,X,Y,LB,RB,\nL3/LSTICK,R3/RSTICK,BACK/SELECT/VIEW,START/MENU,UP/DOWN/LEFT/RIGHT. Default: Y+R3.", "手柄组合键，作用同 toggle_key（打开菜单；菜单禁用时开关所有图标）。\n按键用 + 连接：A,B,X,Y,LB,RB,L3/LSTICK,R3/RSTICK,BACK/SELECT/VIEW,\nSTART/MENU,UP/DOWN/LEFT/RIGHT。默认：Y+R3。", "手把組合鍵，作用同 toggle_key（開啟選單；選單停用時開關所有圖示）。\n按鍵用 + 連接：A,B,X,Y,LB,RB,L3/LSTICK,R3/RSTICK,BACK/SELECT/VIEW,\nSTART/MENU,UP/DOWN/LEFT/RIGHT。預設：Y+R3。"},
        {"debug_logging", "Enable verbose debug logging (memory addresses, param details, FMG internals)", "启用详细调试日志（内存地址、param 详情、FMG 内部信息）。", "啟用詳細偵錯記錄（記憶體位址、param 詳情、FMG 內部資訊）。"},
        {"enable_marker_dump", "Master switch for the marker dump hotkey", "标记导出热键的总开关。", "標記匯出快捷鍵的總開關。"},
        {"marker_dump_key", "Key to dump decoded markers to logs/MapForGoblins_markers.log. Default: F9.", "把已解码标记导出到 logs/MapForGoblins_markers.log 的按键。默认：F9。", "把已解碼標記匯出到 logs/MapForGoblins_markers.log 的按鍵。預設：F9。"},
    };

    template <typename T, size_t N, typename Key, typename Match>
    const T *find_row(const T (&rows)[N], const Key &key, Match match)
    {
        for (const auto &row : rows)
            if (match(row, key))
                return &row;
        return nullptr;
    }

    std::string detect_steam_language()
    {
        HMODULE steam = GetModuleHandleA("steam_api64.dll");
        if (!steam) return "";
        typedef void *(*SteamApps_fn)();
        typedef const char *(*GetLang_fn)(void *);
        auto steamApps = reinterpret_cast<SteamApps_fn>(GetProcAddress(steam, "SteamAPI_SteamApps_v008"));
        auto getLang = reinterpret_cast<GetLang_fn>(GetProcAddress(steam, "SteamAPI_ISteamApps_GetCurrentGameLanguage"));
        if (!steamApps || !getLang) return "";
        void *apps = steamApps();
        if (!apps) return "";
        const char *lang = getLang(apps);
        return (lang && lang[0]) ? lang : "";
    }

    Language cached_auto_language()
    {
        static std::mutex lock;
        static bool cached = false;
        static Language language = Language::English;

        std::lock_guard<std::mutex> guard(lock);
        if (cached)
            return language;

        std::string steam_language = detect_steam_language();
        if (steam_language.empty())
            return Language::English;

        language = goblin::i18n::language_from_steam(steam_language);
        cached = true;
        return language;
    }
}

Language goblin::i18n::language_from_steam(std::string_view steam_language)
{
    std::string s = lower(steam_language);
    if (s == "schinese" || s == "zhocn" || s == "zh-cn" || s == "zh_hans")
        return Language::SimplifiedChinese;
    if (s == "tchinese" || s == "zhotw" || s == "zh-tw" || s == "zh_hant")
        return Language::TraditionalChinese;
    return Language::English;
}

std::string goblin::i18n::normalize_language_config(std::string_view config_value)
{
    std::string s = lower(config_value);
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return c == ' ' || c == '\t'; }), s.end());
    if (s.empty() || s == "auto") return "auto";
    if (s == "english" || s == "en" || s == "engus") return "english";
    if (s == "schinese" || s == "simplified" || s == "simplified_chinese" ||
        s == "zhocn" || s == "zh-cn" || s == "zh_hans") return "schinese";
    if (s == "tchinese" || s == "traditional" || s == "traditional_chinese" ||
        s == "zhotw" || s == "zh-tw" || s == "zh_hant") return "tchinese";
    return "english";
}

Language goblin::i18n::language_from_config(std::string_view config_value)
{
    std::string s = normalize_language_config(config_value);
    if (s == "schinese") return Language::SimplifiedChinese;
    if (s == "tchinese") return Language::TraditionalChinese;
    if (s == "auto") return cached_auto_language();
    return Language::English;
}

Language goblin::i18n::current_language()
{
    return language_from_config(goblin::config::uiLanguage);
}

const char *goblin::i18n::language_code(Language language)
{
    switch (language)
    {
    case Language::SimplifiedChinese: return "schinese";
    case Language::TraditionalChinese: return "tchinese";
    default: return "english";
    }
}

const char *goblin::i18n::language_option_label(std::string_view config_value, Language language)
{
    std::string s = normalize_language_config(config_value);
    if (s == "auto")
        return pick(language, "Auto", "自动", "自動");
    if (s == "schinese")
        return pick(language, "Simplified Chinese", "简体中文", "簡體中文");
    if (s == "tchinese")
        return pick(language, "Traditional Chinese", "繁體中文", "繁體中文");
    return "English";
}

const char *goblin::i18n::language_preview_label(std::string_view config_value, Language language)
{
    std::string s = normalize_language_config(config_value);
    if (s != "auto")
        return language_option_label(s, language);

    switch (language_from_config(config_value))
    {
    case Language::SimplifiedChinese:
        return pick(language, "Auto: Simplified Chinese", "自动：简体中文", "自動：簡體中文");
    case Language::TraditionalChinese:
        return pick(language, "Auto: Traditional Chinese", "自动：繁體中文", "自動：繁體中文");
    default:
        return pick(language, "Auto: English", "自动：English", "自動：English");
    }
}

const char *goblin::i18n::tr(TextId id, Language language)
{
    const TextRow *row = find_row(TEXTS, id, [](const TextRow &row, TextId key) { return row.id == key; });
    return row ? pick(language, row->en, row->sc, row->tc) : "";
}

const wchar_t *goblin::i18n::wtr(ToastId id, Language language)
{
    const ToastRow *row = find_row(TOASTS, id, [](const ToastRow &row, ToastId key) { return row.id == key; });
    return row ? wpick(language, row->en, row->sc, row->tc) : L"";
}

const char *goblin::i18n::section_label(const char *section_name, Language language)
{
    const NameRow *row = find_row(SECTION_LABELS, section_name,
                                  [](const NameRow &row, const char *key) { return std::strcmp(row.key, key) == 0; });
    return row ? pick(language, row->en, row->sc, row->tc) : section_name;
}

const char *goblin::i18n::section_comment(const char *section_name, const char *fallback, Language language)
{
    const NameRow *row = find_row(SECTION_COMMENTS, section_name,
                                  [](const NameRow &row, const char *key) { return std::strcmp(row.key, key) == 0; });
    return row ? pick(language, row->en, row->sc, row->tc) : fallback;
}

const char *goblin::i18n::entry_label(const char *entry_key, Language language)
{
    const NameRow *row = find_row(ENTRY_LABELS, entry_key,
                                  [](const NameRow &row, const char *key) { return std::strcmp(row.key, key) == 0; });
    return row ? pick(language, row->en, row->sc, row->tc) : entry_key;
}

const char *goblin::i18n::entry_comment(const char *entry_key, const char *fallback, Language language)
{
    const NameRow *row = find_row(ENTRY_COMMENTS, entry_key,
                                  [](const NameRow &row, const char *key) { return std::strcmp(row.key, key) == 0; });
    return row ? pick(language, row->en, row->sc, row->tc) : fallback;
}

const char *goblin::i18n::font_glyph_seed_utf8()
{
    static std::string seed;
    if (!seed.empty())
        return seed.c_str();

    auto add = [](const char *text) {
        if (text && text[0])
            seed += text;
    };
    auto add_name_rows = [&](const auto &rows) {
        for (const auto &row : rows)
        {
            add(row.sc);
            add(row.tc);
        }
    };

    for (const auto &row : TEXTS)
    {
        add(row.sc);
        add(row.tc);
    }
    add_name_rows(SECTION_LABELS);
    add_name_rows(SECTION_COMMENTS);
    add_name_rows(ENTRY_LABELS);
    add_name_rows(ENTRY_COMMENTS);

    return seed.c_str();
}
