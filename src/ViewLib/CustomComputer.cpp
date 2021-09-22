#include "ViewLib/CustomComputer.hpp"
#include "ViewLib/MonkeWatch.hpp"

#include "Utils/TextUtils.hpp"
#include "KeyExtension.hpp"
#include "quest-cosmetic-loader/shared/CosmeticLoader.hpp"
#include "GorillaUI/MainViewManager.hpp"
#include "GorillaUI/MainWatchView.hpp"
#include "config.hpp"

#include "GorillaLocomotion/Player.hpp"
#include "GlobalNamespace/GorillaComputer.hpp"
#include "GlobalNamespace/GorillaKeyboardButton.hpp"
#include "GlobalNamespace/TransformFollow.hpp"

#include "UnityEngine/Object.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector3.hpp"

#include "Utils/LoadUtils.hpp"
#include "GorillaUI.hpp"
DEFINE_TYPE(GorillaUI, CustomComputer);

using namespace GorillaUI::Components;
using namespace TextUtils;
using namespace GorillaUI::KeyExtension;

extern Logger& getLogger();

using namespace GlobalNamespace;
using namespace UnityEngine;


namespace GorillaUI
{
    void CustomComputer::Init(GorillaUI::Components::View* initialView)
    {
        instance = this;
        activeViewManager = *il2cpp_utils::New<GorillaUI::MainViewManager*>();
        activeViewManager->computer = this;

        il2cpp_utils::SetFieldValue(activeViewManager, "mainView", initialView);

        activeViewManager->activeView = initialView;
        initialView->computer = this;
        activeViewManager->Activate();
        
        gorillaComputer = this->GetComponent<GorillaComputer*>();
        //gorillaComputer->set_enabled(false);

        Transform* transform = gorillaComputer->get_transform();
        Transform* keyboardTransform = transform->Find(il2cpp_utils::createcsstr("keyboard"));
        GameObject* keyboardGO =  keyboardTransform->get_gameObject();
        
        MeshRenderer* meshRenderer = keyboardGO->GetComponent<MeshRenderer*>();
        Material* material = meshRenderer->get_material();
        Color kbColor = {0.0f, 0.0f, 0.0f};
        
        material->set_color(kbColor);
        ReplaceKeys(); 
        screenInfo = CreateMonitor();
    }

    void CustomComputer::Redraw()
    {
        instance->screenInfo.set_text(instance->activeViewManager->activeView->text);
    }

    CustomScreenInfo CustomComputer::CreateMonitor()
    {
        Transform* transform = get_transform();
        Transform* monitor = transform->Find(il2cpp_utils::createcsstr("monitor"));
        GameObject* monitorGO = monitor->get_gameObject();
        monitorGO->SetActive(false);
        
        using namespace CosmeticsLoader;
        GameObject* newMonitor = nullptr;
        std::string path = "/sdcard/ModData/com.AnotherAxiom.GorillaTag/Mods/MonkeComputer/Monitor.package";
        auto* loader = new CosmeticLoader(path, [&](std::string name, Il2CppObject* obj){
            GameObject* package = (GameObject*)obj;
            newMonitor = package->get_transform()->Find(il2cpp_utils::createcsstr("Monitor"))->get_gameObject();

            // I shouldnt be doing this here but it would crash otherwise so fuck it
            MonkeWatch::SetupWatch(package->get_transform()->Find(il2cpp_utils::createcsstr("BananaWatch"))->get_gameObject());

        }, "_Monitor", il2cpp_utils::GetSystemType("UnityEngine", "GameObject"));
        
        getLogger().info("Monitor ptr: %p", newMonitor);

        newMonitor->set_name(il2cpp_utils::createcsstr("Custom Monitor"));

        Vector3 localScale = {0.4f, 0.4f, 0.4f};
        Vector3 eulerAngles = {0.0f, 90.0f, 0.0f};
        Vector3 position = {-69.0f, 12.11f, -82.8f};

        Transform* newMonitorTransform = newMonitor->get_transform();

        newMonitorTransform->set_localScale(localScale);
        newMonitorTransform->set_eulerAngles(eulerAngles);
        newMonitorTransform->set_position(position);

        CustomScreenInfo info;
        
        info.transform = newMonitorTransform;
        info.text = newMonitor->GetComponentInChildren<UnityEngine::UI::Text*>();
        info.renderer = newMonitorTransform->Find(il2cpp_utils::createcsstr("Monitor"))->get_gameObject()->GetComponent<UnityEngine::MeshRenderer*>();
        info.materials = info.renderer->get_materials();

        info.set_color(config.screenColor);
        info.set_fontSize(80);
        Texture2D* tex = LoadUtils::LoadTextureFromFile(config.lastActiveBackground);
        if (!tex) 
        {
            getLogger().info("File was nullptr, setting white texture");
            tex = Texture2D::get_whiteTexture();
        }
        info.set_texture(tex);
        
        return info;
    }
    
    void CustomComputer::SetBG(float r, float g, float b)
    {
        screenInfo.set_color(Color(r, g, b));
        config.screenColor = {r, g, b};
        SaveConfig();
    }

    void CustomComputer::SetBGTex(Texture2D* tex)
    {
        if (!tex) 
        {
            getLogger().info("File was nullptr, setting white texture");
            tex = Texture2D::get_whiteTexture();
        }

        screenInfo.set_texture(tex);
    }
    
    void CustomComputer::ReplaceKeys()
    {
        keys.clear();

        // hijack the original keys
        Array<GlobalNamespace::GorillaKeyboardButton*>* buttons = this->GetComponentsInChildren<GlobalNamespace::GorillaKeyboardButton*>();
        int length = buttons->Length();

        for (int i = 0; i < length; i++)
        {
            GlobalNamespace::GorillaKeyboardButton* button = buttons->values[i];
            Il2CppString* characterString = button->characterString;
            std::string cppString = characterString ? to_utf8(csstrtostr(characterString)) : "";
            getLogger().info("Getting key %s", cppString.c_str());
            cppString = toLower(cppString);
            EKeyboardKey key;

            if (!NameToKey(cppString, key)) 
            {
                getLogger().info("NameToKey for %s failed", cppString.c_str());
                continue;
            }
            // The actual text for the keys was moved in the heirachy, move it back so that it moves with keys
            std::string buttonName = to_utf8(csstrtostr(button->get_name()));

            for (int pos = buttonName.find(' '); pos != std::string::npos; pos = buttonName.find(' '))
            {
                buttonName.replace(pos, 1, "");
            }

            auto buttonText = button->get_transform()->get_parent()->get_parent()->Find(il2cpp_utils::newcsstr(string_format("Text/%s", toLower(buttonName).c_str())));
            if (buttonText) buttonText->SetParent(button->get_transform());

            GameObject* gameObject = button->get_gameObject();
            GorillaUI::Components::GorillaKeyboardButton* customButton = gameObject->AddComponent<GorillaUI::Components::GorillaKeyboardButton*>();

            customButton->pressTime = button->pressTime;
            customButton->functionKey = button->functionKey;
            Object::DestroyImmediate(button);

            customButton->Init(this, key);
            keys[key] = customButton;
        }

        GorillaUI::Components::GorillaKeyboardButton* enterKey = keys.at(EKeyboardKey::Enter);
        GorillaUI::Components::GorillaKeyboardButton* mKey = keys.at(EKeyboardKey::M);
        GorillaUI::Components::GorillaKeyboardButton* deleteKey = keys.at(EKeyboardKey::Delete);

        GameObject* enterGO = enterKey->get_gameObject();
        GameObject* deleteGO = deleteKey->get_gameObject();
        Vector3 spaceOffset = {2.6f, 0.0f, 3.0f};
        Vector3 deleteOffset = {2.3f, 0.0f, 0.0f};
        
        CreateKey(enterGO, "Space", spaceOffset, EKeyboardKey::Space, "SPACE");
        CreateKey(deleteGO, "Delete", deleteOffset, EKeyboardKey::Delete);

        Color backButtonColor = {0.52f, 0.52f, 0.87f};

        deleteKey->Init(this, EKeyboardKey::Back, "BACK", backButtonColor);

        Color arrowKeyButtonColor = {0.67f, 0.86f, 0.67f};

        Vector3 leftKeyOffset = {0.0f, 0.0f, 5.6f};
        Vector3 rightKeyOffset = {0.0f, 0.0f, 10.2f};
        Vector3 upKeyOffset = {-2.3f, 0.0f, 7.9f};
        Vector3 downKeyOffset = {0.0f, 0.0f, 7.9f};

        // make arrow keys
        GameObject* mKeyGO = mKey->get_gameObject();

        CreateKey(mKeyGO, "Left", leftKeyOffset, EKeyboardKey::Left, "<", arrowKeyButtonColor);
        CreateKey(mKeyGO, "Down", downKeyOffset, EKeyboardKey::Down, "v", arrowKeyButtonColor);
        CreateKey(mKeyGO, "Right", rightKeyOffset, EKeyboardKey::Right, ">", arrowKeyButtonColor);
        CreateKey(mKeyGO, "Up", upKeyOffset, EKeyboardKey::Up, "^", arrowKeyButtonColor);
    }

    GorillaUI::Components::GorillaKeyboardButton* CustomComputer::CreateKeyNoInit(GameObject* prefab, std::string goName, Vector3 offset, EKeyboardKey key)
    {
        if (!prefab) 
        {
            getLogger().error("Prefab for keyboard button was nullptr, returning");
            return nullptr;
        }

        // get transform
        Transform* prefabParent = prefab->get_transform()->get_parent();
        
        //duplicate prefab GO
        GameObject* newKey = UnityEngine::Object::Instantiate(prefab);
        
        // get transform
        Transform* newKeyTransform = newKey->get_transform();
        
        // set parent to prefab parent
        newKeyTransform->SetParent(prefabParent, false);

        // re get transform so it's definitely tthe right one
        newKeyTransform = newKey->get_transform();

        // get localpos
        Vector3 pos = newKeyTransform->get_localPosition();

        // update local pos
        pos.x += offset.x;
        pos.y += offset.y;
        pos.z += offset.z;

        // set local pos
        newKeyTransform->set_localPosition(pos);
        
        // set name
        newKey->set_name(il2cpp_utils::createcsstr(goName));
        
        // add custom key component
        return newKey->GetComponent<GorillaUI::Components::GorillaKeyboardButton*>();
    }

    GorillaUI::Components::GorillaKeyboardButton* CustomComputer::CreateKey(GameObject* prefab, std::string goName, Vector3 Offset, EKeyboardKey key)
    {
        GorillaUI::Components::GorillaKeyboardButton* customKeyboardKey = CreateKeyNoInit(prefab, goName, Offset, key);
        customKeyboardKey->Init(this, key);
        return customKeyboardKey;
    }

    GorillaUI::Components::GorillaKeyboardButton* CustomComputer::CreateKey(GameObject* prefab, std::string goName, Vector3 Offset, EKeyboardKey key, std::string label)
    {
        GorillaUI::Components::GorillaKeyboardButton* customKeyboardKey = CreateKeyNoInit(prefab, goName, Offset, key);
        customKeyboardKey->Init(this, key, label);
        return customKeyboardKey;
    }

    GorillaUI::Components::GorillaKeyboardButton* CustomComputer::CreateKey(GameObject* prefab, std::string goName, Vector3 Offset, EKeyboardKey key, std::string label, Color color)
    {
        GorillaUI::Components::GorillaKeyboardButton* customKeyboardKey = CreateKeyNoInit(prefab, goName, Offset, key);
        customKeyboardKey->Init(this, key, label, color);
        return customKeyboardKey;
    }

    void CustomComputer::PressButton(GorillaUI::Components::GorillaKeyboardButton* button)
    {
        if (activeViewManager) 
        {
            auto* notifyOfKeyPress = il2cpp_functions::class_get_method_from_name(il2cpp_utils::ExtractClass (activeViewManager), "NotifyOfKeyPress", 1);
            if (notifyOfKeyPress)
                il2cpp_utils::RunMethod(activeViewManager, notifyOfKeyPress, (int)button->key);
        }
    }

    CustomComputer* CustomComputer::get_instance()
    {
        return instance;
    }
}