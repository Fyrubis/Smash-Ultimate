/*
    Copyright (c) Arnaud BANNIER and Nicolas BODIN.
    Licensed under the MIT License.
    See LICENSE.md in the project root for license information.
*/

#include "ecs/player/fire_knight_system.h"
#include "ecs/player/player_controller_system.h"
#include "ecs/player/player_utils.h"

void FireKnightSystem::OnFixedUpdate(EntityCommandBuffer &ecb)
{
    auto viewAnim = m_registry.view<
        const FireKnightTag,
        const PlayerAffiliation,
        PlayerController,
        SpriteAnimState>();
    for (auto [entity, affiliation, controller, anim] : viewAnim.each())
    {
        if (controller.isStateUpdated == false) continue;

        PlayerConfig *config = g_gameCommon.GetPlayerConfig(affiliation.playerID);
        if (config == nullptr) continue;

        // TODO - Commencez une animation du type RUN quand l'�tat du personnage
        //        passe � l'�tat PlayerState::RUN.

        // TODO - Plus loin dans le tuto, pour la gestion des attaques,
        //        liez l'animation ATTACK_1 � l'�tat ATTACK_COMBO.

        // TODO - Vraiement plus loin dans le tuto, pour le smash,
        //        liez l'animation SMASH_START � l'�tat SMASH_HOLD,
        //        liez l'animation SMASH_RELEASE � l'�tat SMASH_RELEASE.

        AnimCategory animCat = config->category;
        AnimType type = AnimType::UNDEFINED;
        switch (controller.currState)
        {
        case PlayerState::IDLE: type = AnimType::IDLE; break;
        case PlayerState::JUMP: type = AnimType::JUMP_UP; break;
        case PlayerState::FALL: type = AnimType::JUMP_TOP; break;
        case PlayerState::DEFEND: type = AnimType::DEFEND_START; break;
        case PlayerState::LAUNCHED: type = AnimType::JUMP_UP; break;
        case PlayerState::TAKE_DAMAGE: type = AnimType::TAKE_HIT; break;
        default: break;
        }

        if (type != AnimType::UNDEFINED)
        {
            SpriteAnimUtils::SetAnimation(anim, AnimID_Make(config->category, type));
        }
    }
}

void FireKnightSystem::OnUpdate(EntityCommandBuffer &ecb)
{
    auto viewAnim = m_registry.view<
        const FireKnightTag,
        const PlayerAffiliation,
        const PlayerControllerInput,
        const Transform,
        SpriteAnimState,
        const PlayerController,
        PlayerAnimInfo
    >();
    for (auto [entity, affiliation, input, transform, spriteAnim, controller, animInfo] : viewAnim.each())
    {
        assert(spriteAnim.saveEvents);
        for (int i = 0; i < spriteAnim.eventCount; i++)
        {
            SpriteAnimEvent &animEvent = spriteAnim.events[i];

            if (animEvent.type == SpriteAnimEvent::Type::CYCLE_END)
            {
                OnAnimCycleEnd(entity, animEvent, spriteAnim, affiliation, input, animInfo);
            }
            else if (animEvent.type == SpriteAnimEvent::Type::FRAME_CHANGED)
            {
                OnAnimFrameChanged(entity, animEvent, transform, affiliation, controller, animInfo);
            }
        }
    }
}

void FireKnightSystem::OnAnimFrameChanged(
    entt::entity entity,
    const SpriteAnimEvent &animEvent,
    const Transform &transform,
    const PlayerAffiliation &affiliation,
    const PlayerController &controller,
    PlayerAnimInfo &animInfo)
{
    const int playerID = affiliation.playerID;
    PlayerConfig *config = g_gameCommon.GetPlayerConfig(playerID);
    if (config == nullptr) return;

    QueryFilter filter;
    filter.categoryBits = CATEGORY_ATTACK;
    filter.maskBits = config->otherTeamMask | CATEGORY_ITEM;
    filter.solidOnly = true;

    const float lockTime = 0.2f;
    float s = controller.facingRight ? 1.f : -1.f;
    AnimType animType = AnimID_GetType(animEvent.id);

    if (animType ==  AnimType::ATTACK_1)
    {
        animInfo.autoVelocity = 0.0f;

        if (animEvent.index == 1)
        {
            PlayerUtils::PlaySFXAttack(m_scene, playerID, SFX_SWORD_ATTACK_3, SFXIntensity::NORMAL);
        }
        if (animEvent.index == 2)
        {
            // TODO - Effectuez l'attaque sur la frame d'indice 4 et non celle d'indice 2.

            b2Vec2 position = transform.position;

            Damage damage;
            damage.attackCenter = position;
            damage.amount = 3.f;
            damage.ejectionType = Damage::Type::NO_EJECTION;
            damage.lockTime = lockTime;
            damage.lockAttackTime = 10.5f * PLAYER_ATTACK_FRAME_TIME;

            // TODO - Modifiez le rayon du cercle d'attaque avec la valeur trouv�e
            //        en utilisant les outils de debug dans le jeu.

            const b2Vec2 center = transform.position + b2Vec2{ s * 0.8f, 1.5f };
            const float radius = 0.5f;
            bool hit = DamageUtils::AttackCircle(m_scene, entity, affiliation, damage, filter, center, radius);
            PlayerUtils::PlaySFXHit(m_scene, playerID, SFX_SWORD_HIT_A1, SFXIntensity::NORMAL, hit);
        }
    }
    else if (animType == AnimType::ATTACK_2)
    {
        // TODO - Modifier la vitesse automatique pour lui donner une valeur de 2 unit� par seconde.
        //      - Utilisez la constante s pour modifier la direction.

        if (animEvent.index == 0)
        {
            animInfo.autoVelocity = 15.0f;
        }

        const b2Vec2 position = transform.position;
        Damage damage;
        damage.attackCenter = transform.position;
        damage.amount = 3.f;
        damage.ejectionType = Damage::Type::NO_EJECTION;
        damage.lockTime = lockTime;
        damage.lockAttackTime = 5.5f * PLAYER_ATTACK_FRAME_TIME;

        if (animEvent.index == 1)
        {
            PlayerUtils::PlaySFXAttack(m_scene, playerID, SFX_SWORD_ATTACK_2, SFXIntensity::NORMAL);
        }
        else if (animEvent.index == 3)
        {
            const b2Vec2 vertices[5] = {
                position + b2Vec2(s * -0.f, 2.f),
                position + b2Vec2(s * -2.f, 1.6f),
                position + b2Vec2(s * -2.5f, 1.1f),
                position + b2Vec2(s * -1.4f, 0.f),
                position + b2Vec2(s * -0.f, 0.f),
            };
            bool hit = DamageUtils::AttackPolygon(m_scene, entity, affiliation, damage, filter, vertices, 5);
            PlayerUtils::PlaySFXHit(m_scene, playerID, SFX_SWORD_HIT_B1, SFXIntensity::WEAK, hit);
        }
        else if (animEvent.index == 5)
        {
            const b2Vec2 vertices[5] = {
                position + b2Vec2(s * 0.f, 0.f),
                position + b2Vec2(s * 2.6f, 0.4f),
                position + b2Vec2(s * 3.1f, 1.1f),
                position + b2Vec2(s * 1.4f, 2.2f),
                position + b2Vec2(s * 0.f, 2.4f),
            };
            bool hit = DamageUtils::AttackPolygon(m_scene, entity, affiliation, damage, filter, vertices, 5);
            PlayerUtils::PlaySFXHit(m_scene, playerID, SFX_SWORD_HIT_A3, SFXIntensity::WEAK, hit);
        }
        else if (animEvent.index == 6)
        {
            const b2Vec2 vertices[6] = {
                position + b2Vec2(s * -1.f, 2.2f),
                position + b2Vec2(s * -2.3f, 1.9f),
                position + b2Vec2(s * -2.9f, 1.3f),
                position + b2Vec2(s * -2.2f, 0.3f),
                position + b2Vec2(s * -0.f, 0.f),
                position + b2Vec2(s * -0.f, 2.4f),
            };
            bool hit = DamageUtils::AttackPolygon(m_scene, entity, affiliation, damage, filter, vertices, 6);
            PlayerUtils::PlaySFXHit(m_scene, playerID, SFX_SWORD_HIT_B2, SFXIntensity::WEAK, hit);
        }
        else if (animEvent.index == 7)
        {
            const b2Vec2 vertices[6] = {
                position + b2Vec2(s * 0.f, 0.f),
                position + b2Vec2(s * 2.6f, 0.7f),
                position + b2Vec2(s * 3.2f, 1.2f),
                position + b2Vec2(s * 2.9f, 1.8f),
                position + b2Vec2(s * 1.7f, 2.3f),
                position + b2Vec2(s * 0.f, 2.4f),
            };
            bool hit = DamageUtils::AttackPolygon(m_scene, entity, affiliation, damage, filter, vertices, 6);
            PlayerUtils::PlaySFXHit(m_scene, playerID, SFX_SWORD_HIT_A2, SFXIntensity::WEAK, hit);
        }
    }
    else if (animType == AnimType::ATTACK_3)
    {
        animInfo.autoVelocity = 0.f;

        if (animEvent.index == 2)
        {
            PlayerUtils::PlaySFXAttack(m_scene, playerID, SFX_FIRE_2, SFXIntensity::STRONG);
        }
        else if (animEvent.index == 5)
        {
            // TODO - Modifiez les dommages pour effectuer une ejection.

            b2Vec2 position = transform.position;
            Damage damage;
            damage.attackCenter = position;
            damage.amount = 1.f;
            damage.ejectionType = Damage::Type::NO_EJECTION;
            //damage.direction = math::UnitVectorDeg(90.f - s * 45.f);
            damage.ejectionSpeed = 9.f;
            damage.lockTime = lockTime;
            damage.lockAttackTime = 10.5f * PLAYER_ATTACK_FRAME_TIME;

            const b2Vec2 boxCenter = transform.position + b2Vec2{ s * 2.f, 1.5f };
            bool hit = DamageUtils::AttackBox(m_scene, entity, affiliation, damage, filter, boxCenter, 2.f, 1.5f);
            PlayerUtils::PlaySFXHit(m_scene, playerID, SFX_SWORD_HIT_A1, SFXIntensity::STRONG, hit);
        }
    }
    else if (animType == AnimType::SMASH_RELEASE)
    {
        switch (animEvent.index)
        {
        case 0:  animInfo.autoVelocity = s * +0.0f; break;
        case 2:  animInfo.autoVelocity = s * +9.0f; break;
        case 3:  animInfo.autoVelocity = s * 14.0f; break;
        case 4:  animInfo.autoVelocity = s * +8.0f; break;
        case 5:  animInfo.autoVelocity = s * +0.0f; break;
        case 8:  animInfo.autoVelocity = s * -9.0f; break;
        case 10: animInfo.autoVelocity = s * +0.0f; break;
        default: break;
        }

        if (animEvent.index == 2)
        {
            PlayerUtils::PlaySFXAttack(m_scene, playerID, SFX_FIRE_2, SFXIntensity::MAX);
        }
        else if (animEvent.index == 5)
        {
            Damage damage;
            damage.attackCenter = transform.position + b2Vec2{ s * -2.f , -1.f };
            damage.amount = controller.smashMultiplier * 10.f;
            damage.ejectionType = Damage::Type::CENTER;
            damage.ejectionSpeed = controller.smashMultiplier * 12.f;
            damage.lockTime = lockTime;
            damage.lockAttackTime = 5.5f * PLAYER_ATTACK_FRAME_TIME;

            const b2Vec2 center = transform.position + b2Vec2{ s * 0.4f, 1.5f };
            bool hit = DamageUtils::AttackCircle(m_scene, entity, affiliation, damage, filter, center, 1.8f);
            PlayerUtils::PlaySFXHit(m_scene, playerID, SFX_SWORD_HIT_A2, SFXIntensity::MAX, hit);
        }
    }
    else if (animType == AnimType::RUN)
    {
        switch (animEvent.index)
        {
        case 1:
            PlayerUtils::EmitSmallDust(m_scene, transform.position, controller.facingRight, s * 0.6f);
            break;
        case 5:
            PlayerUtils::EmitSmallDust(m_scene, transform.position, controller.facingRight, s * 0.1f);
            break;
        default: break;
        }
    }
}

void FireKnightSystem::OnAnimCycleEnd(
    entt::entity entity,
    const SpriteAnimEvent &animEvent,
    SpriteAnimState &anim,
    const PlayerAffiliation &affiliation,
    const PlayerControllerInput &input,
    PlayerAnimInfo &event)
{
    PlayerConfig *config = g_gameCommon.GetPlayerConfig(affiliation.playerID);
    if (config == nullptr) return;

    AnimType nextAnimType = AnimType::UNDEFINED;
    switch (AnimID_GetType(animEvent.id))
    {
    case AnimType::DEFEND_START:
    {
        nextAnimType = AnimType::DEFEND;
        break;
    }
    case AnimType::JUMP_TOP:
    {
        nextAnimType = AnimType::JUMP_DOWN;
        break;
    }
    case AnimType::ATTACK_1:
    {
        // TODO - D�commenter l'encha�nement d'animation apr�s la premi�re attaque.

        //if (input.attackDown)
        //{
        //    nextAnimType = AnimType::ATTACK_2;
        //}
        //else
        //{
        //    nextAnimType = AnimType::ATTACK_1_END;
        //}
        break;
    }
    case AnimType::ATTACK_2:
    {
        if (input.attackDown)
        {
            nextAnimType = AnimType::ATTACK_3;
        }
        else
        {
            nextAnimType = AnimType::ATTACK_2_END;
        }
        break;
    }
    case AnimType::ATTACK_1_END:
    case AnimType::ATTACK_2_END:
    case AnimType::ATTACK_3:
    {
        event.type = PlayerAnimInfo::Event::COMBO_END;
        break;
    }
    case AnimType::TAKE_HIT:
    {
        event.type = PlayerAnimInfo::Event::TAKE_HIT_END;
        break;
    }
    // TODO - D�commentez le code suivant.

    //case AnimType::SMASH_START:
    //{
    //    nextAnimType = AnimType::SMASH_HOLD;
    //    break;
    //}
    //case AnimType::SMASH_RELEASE:
    //{
    //    event.type = PlayerAnimInfo::Event::SMASH_END;
    //    break;
    //}
    default: break;
    }

    if (nextAnimType != AnimType::UNDEFINED)
    {
        SpriteAnimUtils::SetAnimation(anim, AnimID_Make(config->category, nextAnimType));
    }
}
