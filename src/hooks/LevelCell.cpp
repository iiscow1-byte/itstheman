#include "../IntegratedDemonlist.hpp"
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/StringBuffer.hpp>
#include <jasmine/hook.hpp>
#include <jasmine/setting.hpp>
#include <jasmine/web.hpp>

using namespace geode::prelude;

std::set<int> loadedDemons;

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

        auto platformer = level->isPlatformer();
        auto difficulty = level->m_demonDifficulty;
        if (level->m_levelType == GJLevelType::Editor || level->m_demon.value() <= 0 ||
            (!platformer && difficulty < 6) || (platformer && difficulty != 0 && difficulty < 5)) return;

        auto levelID = level->m_levelID.value();
        std::vector<int> positions;
        for (auto& demon : platformer ? IntegratedDemonlist::pemonlist : IntegratedDemonlist::aredl) {
            if (demon.id == levelID) positions.push_back(demon.position);
        }
        if (!positions.empty()) return addRank(positions);

        if (!platformer) {
            for (auto& demon : IntegratedDemonlist::allList) {
                if (demon.id == levelID) return addRankALL({ demon.position });
            }
        }

        if (loadedDemons.contains(levelID)) return;
        loadedDemons.insert(levelID);

        m_fields->m_listener.spawn(
            web::WebRequest().get(platformer
                ? fmt::format("https://pemonlist.com/api/level/{}?version=2", levelID)
                : fmt::format("https://list-production-2b7d.up.railway.app/api/v2/demons/?level_id={}&limit=1", levelID)),
            [this, levelID, levelName = std::string(level->m_levelName), platformer](
                web::WebResponse res
            ) mutable {
                if (!res.ok()) return;

                int position1 = -1;
                int tier1 = -1;
                if (platformer) {
                    auto json = res.json();
                    if (!json.isOk()) return;
                    auto pos = json.unwrap().get<int>("placement");
                    if (!pos.isOk()) return;
                    position1 = pos.unwrap();
                    if (position1 > 150) return;
                } else {
                    for (auto& d : jasmine::web::getArray(res)) {
                        auto pos = d.get<int>("position");
                        if (!pos.isOk()) continue;
                        position1 = pos.unwrap();
                        auto t = d.get<int>("tier");
                        if (t.isOk()) tier1 = t.unwrap();
                        break;
                    }
                    if (position1 == -1) return;
                }

                IDListDemon demon(levelID, position1, levelName);
                demon.tier = tier1;
                auto& list = platformer ? IntegratedDemonlist::pemonlist : IntegratedDemonlist::aredl;
                if (!std::ranges::contains(list, demon)) {
                    list.push_back(std::move(demon));
                }

                addRank({ position1 });
            }
        );
    }

    void addRankALL(const std::vector<int>& positions) {
        addRankWithLabel(positions, " ALL");
    }

    void addRank(const std::vector<int>& positions) {
        addRankWithLabel(positions, m_level->isPlatformer() ? " Pemonlist" : " MSCL");
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
