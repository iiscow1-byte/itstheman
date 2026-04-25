#include "../IntegratedDemonlist.hpp"
#include "../classes/IDListLayer.hpp"
#include <Geode/binding/GJDifficultySprite.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/StringBuffer.hpp>
#include <jasmine/hook.hpp>
#include <jasmine/setting.hpp>
#include <jasmine/web.hpp>

using namespace geode::prelude;

std::set<int> loadedDemons;

struct RankResult {
    std::vector<int> positions;
    const char* label = nullptr;
};

static GJDifficulty difficultyForTier(int tier) {
    if (tier > 20) return GJDifficulty::DemonExtreme;
    if (tier >= 15) return GJDifficulty::DemonInsane;
    if (tier >= 10) return GJDifficulty::Demon;
    if (tier >= 5)  return GJDifficulty::DemonMedium;
    return GJDifficulty::DemonEasy;
}

static int findTierAcrossLists(int levelID) {
    for (auto& d : IntegratedDemonlist::aredl)         if (d.id == levelID && d.tier >= 0) return d.tier;
    for (auto& d : IntegratedDemonlist::allList)       if (d.id == levelID && d.tier >= 0) return d.tier;
    for (auto& d : IntegratedDemonlist::aredlOfficial) if (d.id == levelID && d.tier >= 0) return d.tier;
    for (auto& d : IntegratedDemonlist::challengeList) if (d.id == levelID && d.tier >= 0) return d.tier;
    return -1;
}

static RankResult collectFromList(const std::vector<IDListDemon>& list, int levelID, const char* label, bool multiple) {
    RankResult r{ {}, label };
    for (auto& d : list) {
        if (d.id == levelID) {
            r.positions.push_back(d.position);
            if (!multiple) break;
        }
    }
    return r;
}

static RankResult findRank(int levelID, bool showMscl) {
    if (IDListState::inListLayer) {
        switch (IDListState::currentMode) {
            case 0: { auto r = collectFromList(IntegratedDemonlist::aredl,         levelID, " MSCL",  true);  if (!r.positions.empty()) return r; break; }
            case 1: { auto r = collectFromList(IntegratedDemonlist::allList,       levelID, " ALL",   false); if (!r.positions.empty()) return r; break; }
            case 2: { auto r = collectFromList(IntegratedDemonlist::aredlOfficial, levelID, " AREDL", false); if (!r.positions.empty()) return r; break; }
            case 3: { auto r = collectFromList(IntegratedDemonlist::challengeList, levelID, " CL",    false); if (!r.positions.empty()) return r; break; }
        }
    }

    if (showMscl) {
        auto r = collectFromList(IntegratedDemonlist::aredl, levelID, " MSCL", true);
        if (!r.positions.empty()) return r;
    }
    if (auto r = collectFromList(IntegratedDemonlist::aredlOfficial, levelID, " AREDL", false); !r.positions.empty()) return r;
    if (auto r = collectFromList(IntegratedDemonlist::challengeList, levelID, " CL",    false); !r.positions.empty()) return r;
    if (auto r = collectFromList(IntegratedDemonlist::allList,       levelID, " ALL",   false); !r.positions.empty()) return r;
    return {};
}

class $modify(IDLevelCell, LevelCell) {
    struct Fields {
        TaskHolder<web::WebResponse> m_listener;
    };

    static void onModify(ModifyBase<ModifyDerive<IDLevelCell, LevelCell>>& self) {
        (void)self.setHookPriorityAfterPost("LevelCell::loadFromLevel", "hiimjustin000.level_size");
        jasmine::hook::modify(self.m_hooks, "LevelCell::loadFromLevel", "enable-rank");
    }

    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);

        if (level->isPlatformer()) return;
        if (level->m_levelType == GJLevelType::Editor) return;

        auto levelID = level->m_levelID.value();
        bool isRatedDemon = level->m_demon.value() > 0 && level->m_demonDifficulty >= 6;
        bool showMscl = Mod::get()->getSettingValue<bool>("show-mscl");

        int tier = findTierAcrossLists(levelID);
        auto rank = findRank(levelID, showMscl);

        if (!rank.positions.empty()) {
            addRankWithLabel(rank.positions, rank.label, tier);
        } else if (tier >= 0) {
            addTierSprite(tier);
        }

        if (!isRatedDemon && tier > 0) {
            updateDiffSpriteForTier(tier);
        }

        // Rated demon not in any cached list — fetch from MSCL API if enabled.
        if (isRatedDemon && rank.positions.empty() && showMscl) {
            if (loadedDemons.contains(levelID)) return;
            loadedDemons.insert(levelID);

            m_fields->m_listener.spawn(
                web::WebRequest().get(
                    fmt::format("https://list-production-2b7d.up.railway.app/api/v2/demons/?level_id={}&limit=1", levelID)),
                [this, levelID, levelName = std::string(level->m_levelName)](web::WebResponse res) mutable {
                    if (!res.ok()) return;

                    int position = -1;
                    int fetchedTier = -1;
                    for (auto& d : jasmine::web::getArray(res)) {
                        auto pos = d.get<int>("position");
                        if (!pos.isOk()) continue;
                        position = pos.unwrap();
                        auto t = d.get<int>("tier");
                        if (t.isOk()) fetchedTier = t.unwrap();
                        break;
                    }
                    if (position == -1) return;

                    IDListDemon demon(levelID, position, levelName);
                    demon.tier = fetchedTier;
                    if (!std::ranges::contains(IntegratedDemonlist::aredl, demon)) {
                        IntegratedDemonlist::aredl.push_back(std::move(demon));
                    }

                    addRankWithLabel({ position }, " MSCL", fetchedTier);
                }
            );
        }
    }

    void updateDiffSpriteForTier(int tier) {
        auto diffSpr = m_mainLayer->getChildByType<GJDifficultySprite>(0);
        if (!diffSpr) return;
        diffSpr->updateDifficultyFrame((int)difficultyForTier(tier), GJDifficultyName::Short);
    }

    void addTierSprite(int tier) {
        if (tier < 0) return;
        if (m_mainLayer->getChildByID("level-tier-icon"_spr)) return;

        auto tierPath = (Mod::get()->getResourcesDir() / fmt::format("{}-uhd.png", tier)).string();
        auto tierSpr = CCSprite::create(tierPath.c_str());
        if (!tierSpr) return;

        tierSpr->setScale(m_compactView ? 0.22f : 0.28f);
        tierSpr->setAnchorPoint({ 1.0f, 1.0f });
        tierSpr->setPosition({ 352.0f, m_compactView ? 38.0f : 84.0f });
        tierSpr->setID("level-tier-icon"_spr);
        m_mainLayer->addChild(tierSpr, 10);
    }

    void addRankWithLabel(const std::vector<int>& positions, const char* listName, int tier) {
        addTierSprite(tier);
        if (m_mainLayer->getChildByID("level-rank-label"_spr)) return;

        auto dailyLevel = m_level->m_dailyID.value() > 0;
        auto isWhite = dailyLevel || jasmine::setting::getValue<bool>("white-rank");

        StringBuffer<> positionsStr;
        for (auto it = positions.begin(); it != positions.end(); ++it) {
            if (it != positions.begin()) positionsStr.append('/');
            positionsStr.append("#{}", *it);
        }
        positionsStr.append("{}", listName);

        auto rankTextNode = CCLabelBMFont::create(positionsStr.c_str(), "chatFont.fnt");
        rankTextNode->setPosition({ 346.0f, dailyLevel ? 6.0f : 1.0f });
        rankTextNode->setAnchorPoint({ 1.0f, 0.0f });
        rankTextNode->setScale(m_compactView ? 0.45f : 0.6f);
        auto rlc = Loader::get()->getLoadedMod("raydeeux.revisedlevelcells");
        if (rlc && rlc->getSettingValue<bool>("enabled") && rlc->getSettingValue<bool>("blendingText")) {
            rankTextNode->setBlendFunc({ GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA });
        }
        else if (isWhite) {
            rankTextNode->setOpacity(152);
        }
        else {
            rankTextNode->setColor({ 51, 51, 51 });
            rankTextNode->setOpacity(200);
        }
        rankTextNode->setID("level-rank-label"_spr);
        m_mainLayer->addChild(rankTextNode);

        if (auto levelSizeLabel = m_mainLayer->getChildByID("hiimjustin000.level_size/size-label")) {
            levelSizeLabel->setPosition({
                m_compactView ? 343.0f - rankTextNode->getScaledContentWidth() : 346.0f,
                m_compactView ? 1.0f : 12.0f
            });
        }
    }
};
