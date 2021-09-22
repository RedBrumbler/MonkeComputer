#include "ViewLib/GorillaKeyboardButton.hpp"
#include "ViewLib/ViewManager.hpp"
#include <thread>

#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Text.hpp"

#include "GlobalNamespace/GorillaTriggerColliderHandIndicator.hpp"
#include "GlobalNamespace/GorillaTagger.hpp"
DEFINE_TYPE(GorillaUI::Components, GorillaKeyboardButton);

#define KEY_BUMP_AMOUNT 0.2f
#define PRESS_COOLDOWN 150

using namespace UnityEngine;
static const UnityEngine::Color pressedColor = {0.5f, 0.5f, 0.5f};

extern Logger& getLogger();

namespace GorillaUI::Components
{
    void GorillaKeyboardButton::Awake()
    {
        set_enabled(false);
        MeshRenderer* meshRenderer = this->GetComponent<MeshRenderer*>();
        material = meshRenderer->get_material();
        originalColor = material->get_color();
    }

    void GorillaKeyboardButton::Init(GorillaUI::CustomComputer* computer, EKeyboardKey key)
    {
        this->computer = computer;
        this->key = key;
        set_enabled(true);
    }

    void GorillaKeyboardButton::Init(GorillaUI::CustomComputer* computer, EKeyboardKey key, std::string text)
    {
        this->Init(computer, key);
        UI::Text* textComponent = get_gameObject()->GetComponentInChildren<UI::Text*>();
        textComponent->set_text(il2cpp_utils::newcsstr(text));
    }

    void GorillaKeyboardButton::Init(GorillaUI::CustomComputer* computer, EKeyboardKey key, std::string text, Color buttonColor)
    {
        Init(computer, key, text);

        originalColor = buttonColor;
        material->set_color(originalColor);
    }


    void GorillaKeyboardButton::BumpIn()
    {
        Vector3 pos = get_transform()->get_localPosition();
        pos.y -= KEY_BUMP_AMOUNT;
        get_transform()->set_localPosition(pos);
        material->set_color(pressedColor);
    }

    void GorillaKeyboardButton::BumpOut()
    {
        Vector3 pos = get_transform()->get_localPosition();
        pos.y += KEY_BUMP_AMOUNT;
        get_transform()->set_localPosition(pos);
        material->set_color(originalColor);
    }

    void GorillaKeyboardButton::OnTriggerEnter(Collider* collider)
    {
        BumpIn();
        if (isOnCooldown) return;
        isOnCooldown = true;
        
        GlobalNamespace::GorillaTriggerColliderHandIndicator* handIndicator = collider->GetComponentInParent<GlobalNamespace::GorillaTriggerColliderHandIndicator*>();
        if (handIndicator)
		{
            GlobalNamespace::GorillaTriggerColliderHandIndicator* component = collider->GetComponent<GlobalNamespace::GorillaTriggerColliderHandIndicator*>();

			computer->PressButton(this);
			if (component)
			{
				GlobalNamespace::GorillaTagger* gorillaTagger = GlobalNamespace::GorillaTagger::get_Instance();
                bool isLeftHand = component->isLeftHand;
                float tapHapticStrength = gorillaTagger->tapHapticStrength;
                float tapHapticDuration = gorillaTagger->tapHapticDuration;
    
                gorillaTagger->StartVibration(isLeftHand, tapHapticStrength / 2.0f, tapHapticDuration);          
			}
		}

        std::thread cooldown([&]{
            for (int i = 0; i < PRESS_COOLDOWN; i++) usleep(1000);
            isOnCooldown = false;
        });

        cooldown.detach();
    }

    void GorillaKeyboardButton::OnTriggerExit(Collider* collider)
    {
        BumpOut();
    }
}