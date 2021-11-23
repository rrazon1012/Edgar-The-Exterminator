#pragma once

#include "PlayerControlSystem.h"
#include "InputComponent.h"
#include "Animation.h"
#include "Renderer.h"
#include <thread>

PlayerControlSystem::PlayerControlSystem()
{
    invincibleTimer = new b2Timer();
    invincibleTimer->Reset();
    isInvincible = false;
    isInContactWithEnemy = false;
    isRespawning = false;
    health = maxHealth;
}

PlayerControlSystem::~PlayerControlSystem()
{
    delete invincibleTimer;
}

void PlayerControlSystem::processEntity(EntityID id) {
    EntityCoordinator& coordinator = EntityCoordinator::getInstance();


    // don't do anything if the player is deleted
    if (!coordinator.doesEntityExist(id))
    {
        return;
    }

    // Getting Components needed for the player
    Renderer* renderer = Renderer::getInstance();
    InputTracker& input = InputTracker::getInstance();
    PhysicsComponent* physComponent = &coordinator.GetComponent<PhysicsComponent>(id);
    Transform* transformComponent = &coordinator.GetComponent<Transform>(id);
    RenderComponent* renderComponent = &coordinator.GetComponent<RenderComponent>(id);
    MovementComponent* moveComponent = &coordinator.GetComponent<MovementComponent>(id);
    StateComponent* stateComponent = &coordinator.GetComponent<StateComponent>(id);
    AnimationComponent* animationComponent = &coordinator.GetComponent<AnimationComponent>(id);
    GameEntityCreator& creator = GameEntityCreator::getInstance();
    PhysicsWorld& physWorld = PhysicsWorld::getInstance();

    // Setting animations 
    Animation* animRunning = renderer->getAnimation("running", renderComponent->spriteName);
    Animation* animHurting = renderer->getAnimation("hurt", renderComponent->spriteName);
    Animation* animIdle = renderer->getAnimation("idle", renderComponent->spriteName);

    // Character control
    float xVelocity = moveComponent->getVelocity().x;
    float yVelocity = moveComponent->getVelocity().y;
    // state
    int currState = stateComponent->state;

    float speed = 4.0f;
    float jumpForce = 500.0f;
    int jumpCount = 0;
    int jumpLimit = 1;
    bool isReset = false;
    // Colliding with Platform count as ground check 
    //TODO: Improve it with a sensor detector at the bottom of the player to detect if colliding with ground.
    bool isCollided = physComponent->box2dBody->GetContactList();
    //float force = coordinator->GetComponent<PhysicsComponent>(mike).box2dBody->GetMass() * 10 / (1 / 60.0);
    //force /= 3;
    if (yVelocity == 0) {
        // Change to normal state only if previous state was falling(no mid air jump)
        if (currState == stateComponent->state == STATE_JUMPING) {
            stateComponent->state = STATE_NORMAL;
        }
        // Set to moving state if not falling and moving on x axis
        if (xVelocity != 0) {
            stateComponent->state = STATE_MOVING;
        }
    }
    
    //if (input.isKeyDown(InputTracker::S)) {
    //    animationComponent->currAnim = animHurting;
    //    moveComponent->setVelocity(0, 0);
    //}
    // added to test rotation is working
    if (input.isKeyJustDown(InputTracker::R)) {
        int rot = transformComponent->getRotation();
        transformComponent->setRotation(rot+90.0f);
    }
    //if (isCollided) {
    //    isReset = true;
    //    jumpCount = 0;
    //}
    //else {
    //    isReset = false;
    //}
    //if (input.isKeyJustDown(InputTracker::W) && currState != STATE_JUMPING) {
    //    //if (isReset) {
    //        if (jumpCount < jumpLimit) {
    //            moveComponent->addForce(0, jumpForce);
    //            stateComponent->state = STATE_JUMPING;
    //            jumpCount++;
    //        }
    //    //}
    //}

    //if (input.isKeyJustReleased(InputTracker::S)) {
    //    moveComponent->setVelocity(xVelocity, 0);
    //}
    //if (input.isKeyJustReleased(InputTracker::W)) {
    //    if (yVelocity > 0) {
    //        moveComponent->setVelocity(xVelocity, 0);
    //    }
    //}

    if (!input.isKeyDown(InputTracker::A) && !input.isKeyDown(InputTracker::D)) {
        moveComponent->setVelocity(0, yVelocity);
        animationComponent->currFrame = animationComponent->currAnim->endFrame;
        animationComponent->currAnim = animIdle;
    }

    //if (input.isKeyJustDown(InputTracker::J)) {
    //    // create a new entity for bullet
    //    float xPos = (stateComponent->faceRight) ? transformComponent->getPosition().x + transformComponent->getScale().x/2 : transformComponent->getPosition().x - transformComponent->getScale().x / 2;
    //    float yPos = transformComponent->getPosition().y;
    //    EntityID bullet = creator.CreateActor(xPos, yPos, transformComponent->getScale().x / 2, transformComponent->getScale().y / 2, "bullet.png", { Tag::BULLET }, false, 0);

    //    RenderComponent* bulletrenderComp = &coordinator.GetComponent<RenderComponent>(bullet);
    //    bulletrenderComp->flipX = (stateComponent->faceRight) ? true : false;

    //    physWorld.AddObject(bullet);

    //    // set velocity to the bullet entity
    //    PhysicsComponent* bulletPhysComp = &coordinator.GetComponent<PhysicsComponent>(bullet);
    //    b2Vec2 bulletVelocity = (stateComponent->faceRight) ? b2Vec2(5, 0) : b2Vec2(-5, 0);
    //    bulletPhysComp->box2dBody->SetLinearVelocity(bulletVelocity);
    //}
    // Testing output
    //std::cout << "xVelocity: " << xVelocity << std::endl;
    //std::cout << "yVelocity: " << yVelocity << std::endl;

    // Animation, flip, and velocity
    // Play cannot move when respawning
    if (!isRespawning) {
        if (input.isKeyDown(InputTracker::A)) {
            renderComponent->flipX = true;
            animationComponent->currAnim = animRunning;
            moveComponent->setVelocity(-speed, yVelocity);
            stateComponent->faceRight = false;
        }
        if (input.isKeyDown(InputTracker::D)) {
            renderComponent->flipX = false;
            animationComponent->currAnim = animRunning;
            moveComponent->setVelocity(speed, yVelocity);
            stateComponent->faceRight = true;
        }
    }

    // Update isInvincible boolean and play animation
    if (isInvincible) {
        animationComponent->currAnim = animHurting;
        isInvincible = invincibleTimer->GetMilliseconds() < invincibleLength;
        if (!isInvincible) {
            animationComponent->currAnim = animIdle;
            if (isInContactWithEnemy) damaged();
        }
    }

    // respawn player
    if (health == 0) {
        respawn();
    }
}

void PlayerControlSystem::jump()
{
    GameManager gm = GameManager::getInstance();
    EntityCoordinator& coordinator = EntityCoordinator::getInstance();
    StateComponent& stateComponent = coordinator.GetComponent<StateComponent>(gm.PlayerID());

    MovementComponent& moveComponent = coordinator.GetComponent<MovementComponent>(gm.PlayerID());

    float jumpForce = 500.0f;

    if (isGrounded()) {
        moveComponent.addForce(0, jumpForce);
        stateComponent.state = STATE_JUMPING;
    }
}

void PlayerControlSystem::shoot()
{
    GameManager gm = GameManager::getInstance();
    EntityCoordinator& coordinator = EntityCoordinator::getInstance();
    StateComponent& stateComponent = coordinator.GetComponent<StateComponent>(gm.PlayerID());
    Transform& transformComponent = coordinator.GetComponent<Transform>(gm.PlayerID());
    GameEntityCreator& creator = GameEntityCreator::getInstance();
    PhysicsWorld& physWorld = PhysicsWorld::getInstance();

    // create a new entity for bullet
    float xPos = (stateComponent.faceRight) ? transformComponent.getPosition().x + transformComponent.getScale().x / 2 : transformComponent.getPosition().x - transformComponent.getScale().x / 2;
    float yPos = transformComponent.getPosition().y;
    EntityID bullet = creator.CreateActor(xPos, yPos, transformComponent.getScale().x / 2, transformComponent.getScale().y / 2, "bullet.png", { Tag::BULLET }, false, 0);

    RenderComponent* bulletrenderComp = &coordinator.GetComponent<RenderComponent>(bullet);
    bulletrenderComp->flipX = !stateComponent.faceRight;

    physWorld.AddObject(bullet);

    // set velocity to the bullet entity
    PhysicsComponent* bulletPhysComp = &coordinator.GetComponent<PhysicsComponent>(bullet);
    b2Vec2 bulletVelocity = (stateComponent.faceRight) ? b2Vec2(5, 0) : b2Vec2(-5, 0);
    bulletPhysComp->box2dBody->SetLinearVelocity(bulletVelocity);
}

void PlayerControlSystem::damaged()
{
    if (!isInvincible)
    {
        cout << "Player damaged" << endl;
        invincibleTimer->Reset();
        isInvincible = true;

        //logic for decreasing health
        EntityCoordinator* ec = &EntityCoordinator::getInstance();
        std::shared_ptr<EntityQuery> eq = ec->GetEntityQuery({
            ec->GetComponentType<TextComponent>()
            }, { Tag::HEALTH_NUM });

        ComponentIterator<TextComponent> tci(eq);
        health--;
        std::string healthTxt = "X ";
        tci.nextComponent()->setText(healthTxt + std::to_string(health));
    }
    else
    {
        cout << "Player is invincible" << endl;
    }
}

bool PlayerControlSystem::isGrounded()
{
    GameManager gm = GameManager::getInstance();
    EntityCoordinator& coordinator = EntityCoordinator::getInstance();
    PhysicsComponent* physComponentA = &coordinator.GetComponent<PhysicsComponent>(gm.PlayerID());
    b2ContactEdge* contactList = physComponentA->box2dBody->GetContactList();
    
    while (contactList != nullptr) {
        PhysicsComponent* physComponetB = reinterpret_cast<PhysicsComponent*>(contactList->other->GetUserData().pointer);

        if (coordinator.entityHasTag(PLATFORM, physComponetB->entityID) && contactList->contact->GetManifold()->localPoint.y == -0.5) {
            return true;
        }

        contactList = contactList->next;
    }

    return false;
}

bool PlayerControlSystem::isDead()
{
    GameManager gm = GameManager::getInstance();
    EntityCoordinator& coordinator = EntityCoordinator::getInstance();
    PhysicsComponent* physComponentA = &coordinator.GetComponent<PhysicsComponent>(gm.PlayerID());
    b2ContactEdge* contactList = physComponentA->box2dBody->GetContactList();

    while (contactList != nullptr) {
        PhysicsComponent* physComponetB = reinterpret_cast<PhysicsComponent*>(contactList->other->GetUserData().pointer);

        if (coordinator.entityHasTag(FIRE, physComponetB->entityID)) {
            return true;
        }

        contactList = contactList->next;
    }

    return false;
}
void PlayerControlSystem::Receive(Event e, void* args)
{
    switch (e) {
    case(Event::INPUT_JUMP):
        jump();
        break;
    case(Event::INPUT_SHOOT):
        shoot();
        break;
    case(Event::C_START_PLAYER_ENEMY):
        isInContactWithEnemy = true;
        damaged();
        break;
    case(Event::C_END_PLAYER_ENEMY):
        isInContactWithEnemy = false;
        break;
    case(Event::C_PLAYER_FIRE):
        invincibleTimer->Reset();
        health = 0;
        break;
    }
}

void PlayerControlSystem::respawn()
{
    isRespawning = true;
    isInvincible = true;

    GameManager gm = GameManager::getInstance();
    EntityCoordinator& coordinator = EntityCoordinator::getInstance();
    Transform& spawnerTransformComponent = coordinator.GetComponent<Transform>(gm.PlayerRespawnerID());
    float resPosX = spawnerTransformComponent.getPosition().x;
    float resPosY = spawnerTransformComponent.getPosition().y;
    PhysicsComponent* physComponentA = &coordinator.GetComponent<PhysicsComponent>(gm.PlayerID());

    if (invincibleTimer->GetMilliseconds() > respawningTime)
    {
        cout << "Respawning..." << endl;
        physComponentA->box2dBody->SetTransform(b2Vec2(resPosX, resPosY), 0);
        isRespawning = false;
        isInvincible = false;
        isInContactWithEnemy = false;
        health = maxHealth;

        EntityCoordinator* ec = &EntityCoordinator::getInstance();
        std::shared_ptr<EntityQuery> eq = ec->GetEntityQuery({
            ec->GetComponentType<TextComponent>()
            }, { Tag::HEALTH_NUM });

        ComponentIterator<TextComponent> tci(eq);
        std::string healthTxt = "X ";
        tci.nextComponent()->setText(healthTxt + std::to_string(health));
    }
}
