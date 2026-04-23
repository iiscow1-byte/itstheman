#include "../classes/IDListLayer.hpp"
#include <Geode/loader/Mod.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class IDListSelectPopup : public geode::Popup {
public:
    static IDListSelectPopup* create() {
        auto ret = new IDListSelectPopup();
        if (ret->initPopup()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

private:
    bool initPopup() {
        if (!geode::Popup::init(230.f, 175.f)) return false;
        setTitle("Select List");

        auto menu = CCMenu::create();
        menu->setPosition(m_mainLayer->getContentSize() / 2.f - CCPoint { 0.f, 10.f });
        m_mainLayer->addChild(menu);

        // Top row: MSCL (optional), AREDL
        if (Mod::get()->getSettingValue<bool>("show-mscl")) {
            auto msclBg = CCScale9Sprite::create("GJ_button_01.png");
            msclBg->setContentSize({ 82.f, 55.f });
            auto starSpr = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
            starSpr->setScale(1.0f);
            starSpr->setPosition({ 41.f, 35.f });
            msclBg->addChild(starSpr);
            auto msclLabel = CCLabelBMFont::create("MSCL", "bigFont.fnt");
            msclLabel->setScale(0.5f);
            msclLabel->setPosition({ 41.f, 12.f });
            msclBg->addChild(msclLabel);
            auto msclBtn = CCMenuItemSpriteExtra::create(msclBg, this, menu_selector(IDListSelectPopup::onMSCL));
            msclBtn->setPosition({ -48.f, 32.f });
            menu->addChild(msclBtn);
        }

        auto aredlBg = CCScale9Sprite::create("GJ_button_01.png");
        aredlBg->setContentSize({ 82.f, 55.f });
        auto aredlLabel = CCLabelBMFont::create("AREDL", "bigFont.fnt");
        aredlLabel->setScale(0.5f);
        aredlLabel->setPosition({ 41.f, 27.f });
        aredlBg->addChild(aredlLabel);
        auto aredlBtn = CCMenuItemSpriteExtra::create(aredlBg, this, menu_selector(IDListSelectPopup::onAREDL));
        aredlBtn->setPosition({ Mod::get()->getSettingValue<bool>("show-mscl") ? 48.f : 0.f, 32.f });
        menu->addChild(aredlBtn);

        // Bottom row: CL, ALL
        auto clBg = CCScale9Sprite::create("GJ_button_01.png");
        clBg->setContentSize({ 82.f, 55.f });
        auto clLabel = CCLabelBMFont::create("CL", "bigFont.fnt");
        clLabel->setScale(0.65f);
        clLabel->setPosition({ 41.f, 27.f });
        clBg->addChild(clLabel);
        auto clBtn = CCMenuItemSpriteExtra::create(clBg, this, menu_selector(IDListSelectPopup::onCL));
        clBtn->setPosition({ -48.f, -32.f });
        menu->addChild(clBtn);

        auto allBg = CCScale9Sprite::create("GJ_button_01.png");
        allBg->setContentSize({ 82.f, 55.f });
        auto allLabel = CCLabelBMFont::create("ALL", "bigFont.fnt");
        allLabel->setScale(0.65f);
        allLabel->setPosition({ 41.f, 27.f });
        allBg->addChild(allLabel);
        auto allBtn = CCMenuItemSpriteExtra::create(allBg, this, menu_selector(IDListSelectPopup::onAll));
        allBtn->setPosition({ 48.f, -32.f });
        menu->addChild(allBtn);

        return true;
    }

    void onMSCL(CCObject*) {
        onClose(nullptr);
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, IDListLayer::scene(0)));
    }

    void onAREDL(CCObject*) {
        onClose(nullptr);
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, IDListLayer::scene(2)));
    }

    void onCL(CCObject*) {
        onClose(nullptr);
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, IDListLayer::scene(3)));
    }

    void onAll(CCObject*) {
        onClose(nullptr);
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, IDListLayer::scene(1)));
    }
};

class $modify(IDLevelSearchLayer, LevelSearchLayer) {
    bool init(int searchType) {
        if (!LevelSearchLayer::init(searchType)) return false;

        auto demonlistButtonSprite = CircleButtonSprite::createWithSprite("ID_demonBtn_001.png"_spr);
        demonlistButtonSprite->getTopNode()->setScale(1.0f);
        demonlistButtonSprite->setScale(0.8f);
        auto demonlistButton = CCMenuItemSpriteExtra::create(demonlistButtonSprite, this, menu_selector(IDLevelSearchLayer::onDemonlistLevels));
        demonlistButton->setID("demonlist-button"_spr);
        if (auto menu = getChildByID("other-filter-menu")) {
            menu->addChild(demonlistButton);
            menu->updateLayout();
        }

        return true;
    }

    void onDemonlistLevels(CCObject* sender) {
        IDListSelectPopup::create()->show();
    }
};
