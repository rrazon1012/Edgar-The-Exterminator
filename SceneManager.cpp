#include "SceneManager.h"
#include "file_manager.h"
#include "PhysicsComponent.h"
#include "Tags.h"
#include "MovementComponent.h"
#include "StateComponent.h"

// Component Keys
enum eKeys
{
    TAG,
    TRANSFORM,
    RENDER,
    PHYSICS,
    ANIMATION,
    STATE
};

// convert strings to enums here
unordered_map<std::string, eKeys> keyMap = {
    {"tag", TAG},
    {"transform", TRANSFORM},
    {"render", RENDER},
    {"physics", PHYSICS},
    {"animation", ANIMATION},
    {"state", STATE}
};


unordered_map<std::string, Tag> tagMap = {
    {"enemy", ENEMY},
    {"platform", PLATFORM},
    {"player", PLAYER},
    {"star", STAR},
    {"fire", FIRE}
};




// spritesheet map for converting from string to char
unordered_map <std::string, const char*> spriteMap = {
    {"platform.png", "platform.png"},
    {"Giant_Roach.png", "Giant_Roach.png"},
    {"Edgar.png", "Edgar.png"},
    {"star.png", "star.png"},
    {"fire.png", "fire.png"}
};







SceneManager::SceneManager() {
    coordinator = &(EntityCoordinator::getInstance());
    renderer = Renderer::getInstance();
    this->LoadScene("scene.json");
    this->LoadPrefabs("prefab.json");
}


void SceneManager::LoadScene(std::string filename) {
    sceneJsonArray = json::parse(FileManager::readTextFile(filename));
}

void SceneManager::LoadPrefabs(std::string filename) {
    prefabJsonArray = json::parse(FileManager::readTextFile(filename));
    for (auto& prefab : prefabJsonArray.items()) {
        if (prefab.value().contains("setPrefab")) {
            prefabMap.emplace(
                prefab.value()["setPrefab"].get<std::string>(),
                prefab.value()["prefabValues"].get<json>()
                
            );
        }
    }
}

void SceneManager::CreateEntities() {

    // loop through entities

    for (auto& entity : sceneJsonArray.items()) {

        // For setting all the entity and component values
        // See EntityValue class in the SceneManager.h to add more
        EntityValues ev;
        this->ParseEntityValues(ev, entity.value()); // parses json and updates entityvalue object

        // add components to archetype if they're enabled
        if (ev.transformComponent) ev.components.push_back(coordinator->GetComponentType<Transform>());
        if (ev.renderComponent) ev.components.push_back(coordinator->GetComponentType<RenderComponent>());
        if (ev.physicsComponent) ev.components.push_back(coordinator->GetComponentType<PhysicsComponent>());
        if (ev.animationComponent) ev.components.push_back(coordinator->GetComponentType<AnimationComponent>());
        if (ev.movementComponent) ev.components.push_back(coordinator->GetComponentType<MovementComponent>());
        if (ev.stateComponent) ev.components.push_back(coordinator->GetComponentType<StateComponent>());

        Archetype arch = coordinator->GetArchetype(ev.components);
        EntityID ent = coordinator->CreateEntity(arch, ev.spriteName, ev.tags);

        entities.push_back(ent);

        // Set component values from before here
        if (ev.transformComponent) {
            coordinator->GetComponent<Transform>(ent) = {
                    ev.xPos,
                    ev.yPos,
                    ev.rotation,
                    ev.xScale,
                    ev.yScale
            };
        }

        if (ev.renderComponent) {
            coordinator->GetComponent<RenderComponent>(ent) = {
                    "defaultVertShader.vs",
                    "defaultFragShader.fs",
                    ev.spriteName,
                    ev.rowIndex,
                    ev.colIndex,
                    ev.hasAnimation,
                    true
            };
        }

        if (ev.physicsComponent) {
            coordinator->GetComponent<PhysicsComponent>(ent) = {
                ev.bodyType,
                0.5f * ev.yScale,
                0.5f * ev.xScale,
                ev.xPos,
                ev.yPos,
                ev.density,
                ev.friction,
                false
            };

        }

        if (ev.animationComponent) {
            coordinator->GetComponent<AnimationComponent>(ent) =
                Animator::createAnimationComponent(renderer->getAnimation(ev.animName, ev.spriteName), true);
        }

        if (ev.movementComponent) {
            coordinator->GetComponent<MovementComponent>(ent) = {
            0,
            0
            };
        }
        if (ev.stateComponent) {
            coordinator->GetComponent<StateComponent>(ent) = {
            0,
            true
            };
        }

    }
}


void SceneManager::ParseEntityValues(EntityValues& ev, const json& jsonObject) {
    // check for prefabs first
    if (jsonObject.contains("usePrefab")) {
        json e = prefabMap[jsonObject["usePrefab"].get<std::string>()];
        ParseEntityValues(ev, e);
    }

    //loop through components in the entity
    for (auto& component : jsonObject.items()) {
        auto& details = component.value();
        // Set component booleans and set their values in this switch statement
        if (keyMap.find(component.key()) != keyMap.end()) {
            switch (keyMap[component.key()]) {
            case TAG:
                //TODO: allow for multiple tags
                ev.tags = { tagMap[component.value()] };
                break;

            case TRANSFORM:
                ev.transformComponent = true; // add transform to component

                // Values
                ev.xPos = (details.contains("xPos")) // If component Json contains xPos key
                    ? details["xPos"].get<float>() : ev.xPos; // set the xPos to it's value, else keep it the same

                ev.yPos = (details.contains("yPos"))
                    ? details["yPos"].get<float>() : ev.yPos;

                ev.xScale = (details.contains("xScale"))
                    ? details["xScale"].get<float>() : ev.xScale;

                ev.yScale = (details.contains("yScale"))
                    ? details["yScale"].get<float>() : ev.yScale;

                ev.rotation = (details.contains("rotation"))
                    ? details["rotation"].get<float>() : ev.rotation;

                break;

            case RENDER:
                // Entities with Render always have transform
                ev.renderComponent = true;
                ev.transformComponent = true;

                // Values

                // Todo: Stop using a map to convert from string to string
                ev.spriteName = (details.contains("sprite"))
                    ? spriteMap[details["sprite"].get<std::string>()] : ev.spriteName;

                ev.hasAnimation = details.contains("hasAnim")
                    ? details["hasAnim"].get<bool>() : ev.hasAnimation;

                ev.rowIndex = details.contains("rowIndex")
                    ? details["rowIndex"].get<int>() : ev.rowIndex;

                ev.colIndex = details.contains("colIndex")
                    ? details["colIndex"].get<int>() : ev.colIndex;

                break;

            case PHYSICS:
                // Entties with Physics always have these components
                ev.physicsComponent = true;
                ev.transformComponent = true;
                ev.movementComponent = true;
                

                // TODO: do more than just check for one string
                ev.bodyType = details.contains("b2bodytype") && details["b2bodytype"].get<string>() == "b2_dynamicBody"
                    ? b2_dynamicBody : ev.bodyType;

                ev.friction = details.contains("friction")
                    ? details["friction"].get<float>() : ev.friction;

                ev.density = details.contains("density")
                    ? details["density"].get<float>() : ev.density;

                break;

            case ANIMATION:
                ev.animationComponent = true;
                ev.renderComponent = true;

                ev.animIsPlaying = details.contains("isPlaying")
                    ? details["isPlaying"].get<bool>() : ev.animIsPlaying;

                ev.animName = details.contains("animName")
                    ? details["animName"].get<std::string>() : ev.animName;

                break;

            case STATE:
                ev.stateComponent = true;

                break;
            }
        }
    }

};
