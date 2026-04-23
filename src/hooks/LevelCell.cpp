#include "../IntegratedDemonlist.hpp"
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

static GJDifficulty difficultyForTier(int tier) {
    if (tier > 20) return GJDifficulty::DemonExtreme;
    if (tier >= 15) return GJDifficulty::DemonInsane;
    if (tier >= 10) return GJDifficulty::Demon;
    if (tier >= 5)  return GJDifficulty::DemonMedium;
    return GJDifficulty::DemonEasy;
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
        auto difficulty = level->m_demonDifficulty;
        bool isRatedDemon = level->m_demon.value() > 0 && difficulty >= 6;

        bool showMscl = Mod::get()->getSettingValue<bool>("show-mscl");
        if (isRatedDemon) {
            std::vector<int> positions;
            if (showMscl) {
                for (auto& demon : IntegratedDemonlist::aredl) {
                    if (demon.id == levelID) positions.push_back(demon.position);
                }
                if (!positions.empty()) return addRank(positions);
            }

            for (auto& demon : IntegratedDemonlist::aredlOfficial) {
                if (demon.id == levelID) { positions.push_back(demon.position); break; }
            }
            if (!positions.empty()) return addRankAREDL(positions);

            for (auto& demon : IntegratedDemonlist::challengeList) {
                if (demon.id == levelID) { positions.push_back(demon.position); break; }
            }
            if (!positions.empty()) return addRankCL(positions);

            for (auto& demon : IntegratedDemonlist::allList) {
                if (demon.id == levelID) return addRankALL({ demon.position });
            }

            if (!showMscl) return;
            if (loadedDemons.contains(levelID)) return;
            loadedDemons.insert(levelID);

            m_fields->m_listener.spawn(
                web::WebRequest().get(
                    fmt::format("https://list-production-2b7d.up.railway.app/api/v2/demons/?level_id={}&limit=1", levelID)),
                [this, levelID, levelName = std::string(level->m_levelName)](
                    web::WebResponse res
                ) mutable {
                    if (!res.ok()) return;

                    int position1 = -1;
                    int tier1 = -1;
                    for (auto& d : jasmine::web::getArray(res)) {
                        auto pos = d.get<int>("position");
                        if (!pos.isOk()) continue;
                        position1 = pos.unwrap();
                        auto t = d.get<int>("tier");
                        if (t.isOk()) tier1 = t.unwrap();
                        break;
                    }
                    if (position1 == -1) return;

                    IDListDemon demon(levelID, position1, levelName);
                    demon.tier = tier1;
                    if (!std::ranges::contains(IntegratedDemonlist::aredl, demon)) {
                        IntegratedDemonlist::aredl.push_back(std::move(demon));
                    }

                    addRank({ position1 });
                }
            );
        } else {
            // Unrated level — change difficulty icon if it has a valid tier in any demonlist
            int tier = -1;
            if (showMscl) {
                for (auto& demon : IntegratedDemonlist::aredl) {
                    if (demon.id == levelID) { tier = demon.tier; break; }
                }
            }
            if (tier <= 0) for (auto& demon : IntegratedDemonlist::aredlOfficial) {
                if (demon.id == levelID) { tier = demon.tier; break; }
            }
            if (tier <= 0) for (auto& demon : IntegratedDemonlist::challengeList) {
                if (demon.id == levelID) { tier = demon.tier; break; }
            }
            if (tier <= 0) for (auto& demon : IntegratedDemonlist::allList) {
                if (demon.id == levelID) { tier = demon.tier; break; }
            }
            if (tier > 0) updateDiffSpriteForTier(tier);
        }
    }

    void updateDiffSpriteForTier(int tier) {
        auto diffSpr = m_mainLayer->getChildByType<GJDifficultySprite>(0);
        if (!diffSpr) return;
        diffSpr->updateDifficultyFrame((int)difficultyForTier(tier), GJDifficultyName::Short);
    }

    void addRankALL(const std::vector<int>& positions) {
        addRankWithLabel(positions, " ALL");
    }

    void addRankAREDL(const std::vector<int>& positions) {
        addRankWithLabel(positions, " AREDL");
    }

    void addRankCL(const std::vector<int>& positions) {
        addRankWithLabel(positions, " CL");
    }

    void addRank(const std::vector<int>& positions) {
        addRankWithLabel(positions, " MSCL");
    }

    void addRankWithLabel(const std::vector<int>& positions, const char* listName) {
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
