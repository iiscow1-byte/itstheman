#include "../IntegratedDemonlist.hpp"
#include <Geode/binding/GJDifficultySprite.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <jasmine/web.hpp>

using namespace geode::prelude;

class $modify(IDLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        TaskHolder<web::WebResponse> m_listener;
    };

    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        auto levelID = level->m_levelID.value();
        auto platformer = level->isPlatformer();

        if (platformer) {
            for (auto& demon : IntegratedDemonlist::pemonlist) {
                if (demon.id == levelID) {
                    addDemonlistBadge(demon.position, demon.tier, true);
                    return true;
                }
            }
            if (IntegratedDemonlist::pemonlistLoaded) return true;
        } else {
            for (auto& demon : IntegratedDemonlist::aredl) {
                if (demon.id == levelID) {
                    addDemonlistBadge(demon.position, demon.tier, false);
                    return true;
                }
            }
            for (auto& demon : IntegratedDemonlist::allList) {
                if (demon.id == levelID) {
                    addDemonlistBadge(demon.position, demon.tier, false, "ALL");
                    return true;
                }
            }
            if (IntegratedDemonlist::aredlLoaded) return true;
        }

        m_fields->m_listener.spawn(
            web::WebRequest().get(platformer
                ? fmt::format("https://pemonlist.com/api/level/{}?version=2", levelID)
                : fmt::format("https://list-production-2b7d.up.railway.app/api/v2/demons/?level_id={}&limit=1", levelID)),
            [this, platformer](web::WebResponse res) mutable {
                if (!res.ok()) return;

                int position = -1;
                int tier = -1;

                if (platformer) {
                    auto json = res.json();
                    if (!json.isOk()) return;
                    auto pos = json.unwrap().get<int>("placement");
                    if (!pos.isOk()) return;
                    position = pos.unwrap();
                    if (position > 150) return;
                } else {
                    for (auto& d : jasmine::web::getArray(res)) {
                        auto pos = d.get<int>("position");
                        if (!pos.isOk()) continue;
                        position = pos.unwrap();
                        auto t = d.get<int>("tier");
                        if (t.isOk()) tier = t.unwrap();
                        break;
                    }
                    if (position == -1) return;
                }

                addDemonlistBadge(position, tier, platformer);
            }
        );

        return true;
    }

    void addDemonlistBadge(int position, int tier, bool platformer, const char* listName = nullptr) {
        auto diffSpr = m_difficultySprite;
        if (!diffSpr) return;
        auto parent = diffSpr->getParent();
        if (!parent) return;

        auto diffPos = diffSpr->getPosition();
        float diffHalfW = diffSpr->getScaledContentWidth() * 0.5f;
        int z = diffSpr->getZOrder();

        // Tier icon to the left of the difficulty sprite (classic list only)
        if (!platformer && tier >= 0) {
            auto tierPath = (Mod::get()->getResourcesDir() / fmt::format("{}-uhd.png", tier)).string();
            auto tierSpr = CCSprite::create(tierPath.c_str());
            if (tierSpr) {
                tierSpr->setScale(4.5f / 4.0f);
                tierSpr->setPosition({diffPos.x - diffHalfW - 34.0f, diffPos.y + 8.0f});
                tierSpr->setID("demonlist-tier-icon"_spr);
                parent->addChild(tierSpr, z);
            }
        }

        // Position label below the stars label if it exists
        float rankY = diffPos.y - 45.0f;
        if (m_starsLabel) {
            rankY = m_starsLabel->getPositionY() - m_starsLabel->getScaledContentHeight() - 2.0f;
        }

        const char* name = listName ? listName : (platformer ? "Pemonlist" : "MSCL");
        auto rankLabel = CCLabelBMFont::create(
            fmt::format("#{} {}", position, name).c_str(), "bigFont.fnt"
        );
        rankLabel->setScale(0.45f);
        rankLabel->setColor({255, 200, 50});
        rankLabel->setPosition({diffPos.x, rankY});
        rankLabel->setID("demonlist-rank-label"_spr);
        parent->addChild(rankLabel, z);
    }
};
