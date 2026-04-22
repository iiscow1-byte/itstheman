#include "../IntegratedDemonlist.hpp"
#include <Geode/binding/GJDifficultySprite.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <jasmine/web.hpp>

using namespace geode::prelude;

struct RankInfo {
    int position;
    int tier;
    const char* listName;
};

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
                    addDemonlistBadges(true, {{demon.position, demon.tier, "Pemonlist"}});
                    return true;
                }
            }
            if (IntegratedDemonlist::pemonlistLoaded) return true;
        } else {
            std::vector<RankInfo> ranks;
            for (auto& demon : IntegratedDemonlist::aredl) {
                if (demon.id == levelID) { ranks.push_back({demon.position, demon.tier, "MSCL"}); break; }
            }
            for (auto& demon : IntegratedDemonlist::allList) {
                if (demon.id == levelID) { ranks.push_back({demon.position, demon.tier, "ALL"}); break; }
            }
            if (!ranks.empty()) { addDemonlistBadges(false, ranks); return true; }
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

                addDemonlistBadges(platformer, {{position, tier, platformer ? "Pemonlist" : "MSCL"}});
            }
        );

        return true;
    }

    void addDemonlistBadges(bool platformer, const std::vector<RankInfo>& entries) {
        auto diffSpr = m_difficultySprite;
        if (!diffSpr) return;
        auto parent = diffSpr->getParent();
        if (!parent) return;

        auto diffPos = diffSpr->getPosition();
        float diffHalfW = diffSpr->getScaledContentWidth() * 0.5f;
        int z = diffSpr->getZOrder();

        // Tier icon to the left of the difficulty sprite (use first entry with a valid tier)
        CCSprite* tierSpr = nullptr;
        if (!platformer) {
            for (auto& entry : entries) {
                if (entry.tier >= 0) {
                    auto tierPath = (Mod::get()->getResourcesDir() / fmt::format("{}-uhd.png", entry.tier)).string();
                    tierSpr = CCSprite::create(tierPath.c_str());
                    if (tierSpr) {
                        tierSpr->setScale(4.5f / 4.0f);
                        tierSpr->setPosition({diffPos.x - diffHalfW - 34.0f, diffPos.y + 8.0f});
                        tierSpr->setID("demonlist-tier-icon"_spr);
                        parent->addChild(tierSpr, z);
                    }
                    break;
                }
            }
        }

        // Rank labels stack below the tier icon, right-aligned to the left of the difficulty sprite
        float rightX = diffPos.x - diffHalfW - 4.0f;
        float currentY = tierSpr
            ? tierSpr->getPositionY() - tierSpr->getScaledContentHeight() * 0.5f - 6.0f
            : diffPos.y - 45.0f;

        for (auto& entry : entries) {
            auto rankLabel = CCLabelBMFont::create(
                fmt::format("#{} {}", entry.position, entry.listName).c_str(), "bigFont.fnt"
            );
            rankLabel->setScale(0.45f);
            rankLabel->setColor({255, 200, 50});
            rankLabel->setAnchorPoint({1.0f, 0.5f});
            rankLabel->setPosition({rightX, currentY});
            rankLabel->setID("demonlist-rank-label"_spr);
            parent->addChild(rankLabel, z);
            currentY -= rankLabel->getScaledContentHeight() + 2.0f;
        }
    }
};
