#include "../classes/IDListLayer.hpp"
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
        if (!geode::Popup::init(250.f, 130.f)) return false;
        setTitle("Select List");

        auto menu = CCMenu::create();
        menu->setPosition(m_mainLayer->getContentSize() / 2.f - CCPoint { 0.f, 8.f });
        m_mainLayer->addChild(menu);

        auto msclBg = CCScale9Sprite::create("GJ_button_01.png");
        msclBg->setContentSize({ 90.f, 60.f });
        auto starSpr = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        starSpr->setScale(1.1f);
        starSpr->setPosition({ 45.f, 38.f });
        msclBg->addChild(starSpr);
        auto msclLabel = CCLabelBMFont::create("MSCL", "bigFont.fnt");
        msclLabel->setScale(0.5f);
        msclLabel->setPosition({ 45.f, 13.f });
        msclBg->addChild(msclLabel);
        auto msclBtn = CCMenuItemSpriteExtra::create(msclBg, this, menu_selector(IDListSelectPopup::onMSCL));
        msclBtn->setPosition({ -55.f, 0.f });
        menu->addChild(msclBtn);

        auto pemonBg = CCScale9Sprite::create("GJ_button_01.png");
        pemonBg->setContentSize({ 90.f, 60.f });
        auto moonSpr = CCSprite::createWithSpriteFrameName("GJ_moonsIcon_001.png");
        moonSpr->setScale(1.1f);
        moonSpr->setPosition({ 45.f, 38.f });
        pemonBg->addChild(moonSpr);
        auto pemonLabel = CCLabelBMFont::create("Pemonlist", "bigFont.fnt");
        pemonLabel->setScale(0.35f);
        pemonLabel->setPosition({ 45.f, 13.f });
        pemonBg->addChild(pemonLabel);
        auto pemonBtn = CCMenuItemSpriteExtra::create(pemonBg, this, menu_selector(IDListSelectPopup::onPemonlist));
        pemonBtn->setPosition({ 55.f, 0.f });
        menu->addChild(pemonBtn);

        return true;
    }

    void onMSCL(CCObject*) {
        onClose(nullptr);
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, IDListLayer::scene(false)));
    }

    void onPemonlist(CCObject*) {
        onClose(nullptr);
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, IDListLayer::scene(true)));
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
