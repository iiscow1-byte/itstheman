#include "../IntegratedDemonlist.hpp"
#include <Geode/binding/GJDifficultySprite.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <jasmine/web.hpp>

using namespace geode::prelude;

struct RankInfo {
    int position;
    int tier;
    const char* listName;
};

static GJDifficulty difficultyForTier(int tier) {
    if (tier > 20) return GJDifficulty::DemonExtreme;
    if (tier >= 15) return GJDifficulty::DemonInsane;
    if (tier >= 10) return GJDifficulty::Demon;
    if (tier >= 5)  return GJDifficulty::DemonMedium;
    return GJDifficulty::DemonEasy;
}

static const char* nameForTier(int tier) {
    if (tier > 20) return "Unrated Extreme Demon";
    if (tier >= 15) return "Unrated Insane Demon";
    if (tier >= 10) return "Unrated Hard Demon";
    if (tier >= 5)  return "Unrated Medium Demon";
    return "Unrated Easy Demon";
}

class $modify(IDLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        TaskHolder<web::WebResponse> m_listener;
        std::vector<RankInfo> m_ranks;
        bool m_isUnrated = false;
    };

    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        auto levelID = level->m_levelID.value();
        bool isUnrated = level->m_demon.value() <= 0;
        m_fields->m_isUnrated = isUnrated;

        if (!level->isPlatformer()) {
            bool showMscl = Mod::get()->getSettingValue<bool>("show-mscl");
            std::vector<RankInfo> ranks;
            if (showMscl) {
                for (auto& demon : IntegratedDemonlist::aredl) {
                    if (demon.id == levelID) { ranks.push_back({demon.position, demon.tier, "MSCL"}); break; }
                }
            }
            for (auto& demon : IntegratedDemonlist::aredlOfficial) {
                if (demon.id == levelID) { ranks.push_back({demon.position, demon.tier, "AREDL"}); break; }
            }
            for (auto& demon : IntegratedDemonlist::challengeList) {
                if (demon.id == levelID) { ranks.push_back({demon.position, demon.tier, "CL"}); break; }
            }
            for (auto& demon : IntegratedDemonlist::allList) {
                if (demon.id == levelID) { ranks.push_back({demon.position, demon.tier, "ALL"}); break; }
            }
            if (!ranks.empty()) {
                m_fields->m_ranks = ranks;
                addDemonlistBadges(false, ranks, isUnrated);
                return true;
            }
            if (IntegratedDemonlist::aredlLoaded || !showMscl) return true;

            m_fields->m_listener.spawn(
                web::WebRequest().get(
                    fmt::format("https://list-production-2b7d.up.railway.app/api/v2/demons/?level_id={}&limit=1", levelID)),
                [this, isUnrated](web::WebResponse res) mutable {
                    if (!res.ok()) return;

                    int position = -1;
                    int tier = -1;
                    for (auto& d : jasmine::web::getArray(res)) {
                        auto pos = d.get<int>("position");
                        if (!pos.isOk()) continue;
                        position = pos.unwrap();
                        auto t = d.get<int>("tier");
                        if (t.isOk()) tier = t.unwrap();
                        break;
                    }
                    if (position == -1) return;

                    m_fields->m_ranks = {{position, tier, "MSCL"}};
                    addDemonlistBadges(false, {{position, tier, "MSCL"}}, isUnrated);
                }
            );
        }

        return true;
    }

    void levelDownloadFinished(GJGameLevel* level) override {
        LevelInfoLayer::levelDownloadFinished(level);
        if (!m_fields->m_ranks.empty()) {
            // Re-check rated status from refreshed level data
            m_fields->m_isUnrated = m_level->m_demon.value() <= 0;
            addDemonlistBadges(false, m_fields->m_ranks, m_fields->m_isUnrated);
        }
    }

    void addDemonlistBadges(bool platformer, const std::vector<RankInfo>& entries, bool isUnrated = false) {
        auto diffSpr = m_difficultySprite;
        if (!diffSpr) return;
        auto parent = diffSpr->getParent();
        if (!parent) return;

        // Remove any previously added badges to avoid duplicates on re-apply
        while (auto node = parent->getChildByID("demonlist-tier-icon"_spr)) node->removeFromParent();
        while (auto node = parent->getChildByID("demonlist-rank-label"_spr)) node->removeFromParent();

        auto diffPos = diffSpr->getPosition();
        int z = diffSpr->getZOrder();

        if (isUnrated && !platformer) {
            int tier = -1;
            for (auto& entry : entries) {
                if (entry.tier > 0) { tier = entry.tier; break; }
            }
            if (tier > 0) {
                diffSpr->updateDifficultyFrame((int)difficultyForTier(tier), GJDifficultyName::Long);
                if (auto diffLabel = typeinfo_cast<CCLabelBMFont*>(parent->getChildByID("difficulty-label"))) {
                    diffLabel->setString(nameForTier(tier));
                }
                // Shift coins down to prevent overlap with the longer difficulty label
                if (m_coins) {
                    for (uint32_t i = 0; i < m_coins->count(); i++) {
                        if (auto coin = typeinfo_cast<CCNode*>(m_coins->objectAtIndex(i))) {
                            coin->setPositionY(coin->getPositionY() - 15.0f);
                        }
                    }
                }
            }
        }

        float diffHalfW = diffSpr->getScaledContentWidth() * 0.5f;

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
