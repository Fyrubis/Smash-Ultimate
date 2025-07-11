/*
    Copyright (c) Arnaud BANNIER and Nicolas BODIN.
    Licensed under the MIT License.
    See LICENSE.md in the project root for license information.
*/

#pragma once

#include "game_engine_settings.h"
#include "ui/base/ui_selectable.h"
#include "ui/visual/ui_text.h"
#include "ui/visual/ui_image.h"

class UIButton : public UISelectable
{
public:
    UIButton(Scene *scene, UISelectableGroup *group = nullptr);
    virtual ~UIButton();

    void AddLabelObject(UIObject *object);
    void AddLabelObject(UIObject *object, const UIStateModifiers &stateModifiers);
    void RemoveLabelObject(UIObject *object);

    void AddStateObject(State state, UIObject *object, bool fade = true);
    void RemoveStateObject(State state, UIObject *object);

    virtual void Update() override;
    virtual void OnStateChanged(State state, State prevState, bool makeSound) override;
    virtual void DrawImGui() override;

protected:
    struct Label
    {
        Label() :
            object(nullptr), stateModifiers()
        {}

        UIObject *object;
        UIStateModifiers stateModifiers;
    };

    std::map<UIObject *, Label> m_labelMap;
    std::array<std::map<UIObject *, bool>, 4> m_stateObjectMaps;

private:
    void AddLabelObject(UIObject *object, Label &label);
};
