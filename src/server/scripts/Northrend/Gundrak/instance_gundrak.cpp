/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "gundrak.h"

#define MAX_ENCOUNTER     5

/* GunDrak encounters:
0 - Slad'Ran
1 - Moorabi
2 - Drakkari Colossus
3 - Gal'Darah
4 - Eck the Ferocious
*/

class instance_gundrak : public InstanceMapScript
{
public:
    instance_gundrak() : InstanceMapScript("instance_gundrak", 604) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_gundrak_InstanceMapScript(map);
    }

    struct instance_gundrak_InstanceMapScript : public InstanceScript
    {
        instance_gundrak_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            bHeroicMode = map->IsHeroic();
        }

        bool bHeroicMode;
        bool spawnSupport;

        uint32 timer;
        uint32 phase;
        ObjectGuid toActivate;

        ObjectGuid uiSladRan;
        ObjectGuid uiMoorabi;
        ObjectGuid uiDrakkariColossus;
        ObjectGuid uiGalDarah;
        ObjectGuid uiEckTheFerocious;

        ObjectGuid uiSladRanAltar;
        ObjectGuid uiMoorabiAltar;
        ObjectGuid uiDrakkariColossusAltar;
        ObjectGuid uiSladRanStatue;
        ObjectGuid uiMoorabiStatue;
        ObjectGuid uiDrakkariColossusStatue;
        ObjectGuid uiGalDarahStatue;
        ObjectGuid uiEckTheFerociousDoor;
        ObjectGuid uiEckTheFerociousDoorBehind;
        ObjectGuid uiGalDarahDoor1;
        ObjectGuid uiGalDarahDoor2;
        ObjectGuid uiBridge;
        ObjectGuid uiCollision;

        uint32 m_auiEncounter[MAX_ENCOUNTER];

        GOState uiSladRanStatueState;
        GOState uiMoorabiStatueState;
        GOState uiDrakkariColossusStatueState;
        GOState uiGalDarahStatueState;
        GOState uiBridgeState;
        GOState uiCollisionState;

        GuidSet DwellerGUIDs;

        std::string str_data;

        void Initialize() override
        {
            spawnSupport = false;

            timer = 0;
            phase = 0;
            toActivate.Clear();

            uiSladRan.Clear();
            uiMoorabi.Clear();
            uiDrakkariColossus.Clear();
            uiGalDarah.Clear();
            uiEckTheFerocious.Clear();

            uiSladRanAltar.Clear();
            uiMoorabiAltar.Clear();
            uiDrakkariColossusAltar.Clear();

            uiSladRanStatue.Clear();
            uiMoorabiStatue.Clear();
            uiDrakkariColossusStatue.Clear();
            uiGalDarahStatue.Clear();

            uiEckTheFerociousDoor.Clear();
            uiEckTheFerociousDoorBehind.Clear();
            uiGalDarahDoor1.Clear();
            uiGalDarahDoor2.Clear();

            uiBridge.Clear();
            uiCollision.Clear();

            uiSladRanStatueState = GO_STATE_ACTIVE;
            uiMoorabiStatueState = GO_STATE_ACTIVE;
            uiDrakkariColossusStatueState = GO_STATE_ACTIVE;
            uiGalDarahStatueState = GO_STATE_READY;
            uiBridgeState = GO_STATE_ACTIVE;
            uiCollisionState = GO_STATE_READY;

            DwellerGUIDs.clear();

            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

       bool IsEncounterInProgress() const override
       {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS)
                    return true;

            return false;
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case CREATURE_SLAD_RAN: uiSladRan = creature->GetGUID(); break;
                case CREATURE_MOORABI: uiMoorabi = creature->GetGUID(); break;
                case CREATURE_GALDARAH: uiGalDarah = creature->GetGUID(); break;
                case CREATURE_DRAKKARICOLOSSUS: uiDrakkariColossus = creature->GetGUID(); break;
                case CREATURE_ECK: uiEckTheFerocious = creature->GetGUID(); break;
                case CREATURE_RUIN_DWELLER:
                    if (creature->IsAlive())
                        DwellerGUIDs.insert(creature->GetGUID());
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case 192518:
                    uiSladRanAltar = go->GetGUID();
                    // Make sure that they start out as unusuable
                    go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    if (m_auiEncounter[0] == DONE)
                    {
                        if (uiSladRanStatueState == GO_STATE_ACTIVE)
                            go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        else
                        {
                            ++phase;
                            go->SetGoState(GO_STATE_ACTIVE);
                        }
                    }
                    break;
                case 192519:
                    uiMoorabiAltar = go->GetGUID();
                    // Make sure that they start out as unusuable
                    go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    if (m_auiEncounter[0] == DONE)
                    {
                        if (uiMoorabiStatueState == GO_STATE_ACTIVE)
                            go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        else
                        {
                            ++phase;
                            go->SetGoState(GO_STATE_ACTIVE);
                        }
                    }
                    break;
                case 192520:
                    uiDrakkariColossusAltar = go->GetGUID();
                    // Make sure that they start out as unusuable
                    go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    if (m_auiEncounter[0] == DONE)
                    {
                        if (uiDrakkariColossusStatueState == GO_STATE_ACTIVE)
                            go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        else
                        {
                            ++phase;
                            go->SetGoState(GO_STATE_ACTIVE);
                        }
                    }
                    break;
                case 192564:
                    uiSladRanStatue = go->GetGUID();
                    go->SetGoState(uiSladRanStatueState);
                    break;
                case 192565:
                    uiMoorabiStatue = go->GetGUID();
                    go->SetGoState(uiMoorabiStatueState);
                    break;
                case 192566:
                    uiGalDarahStatue = go->GetGUID();
                    go->SetGoState(uiGalDarahStatueState);
                    break;
                case 192567:
                    uiDrakkariColossusStatue = go->GetGUID();
                    go->SetGoState(uiDrakkariColossusStatueState);
                    break;
                case 192632:
                    uiEckTheFerociousDoor = go->GetGUID();
                    if (bHeroicMode && m_auiEncounter[1] == DONE)
                        HandleGameObject(ObjectGuid::Empty, true, go);
                    break;
                case 192569:
                    uiEckTheFerociousDoorBehind = go->GetGUID();
                    if (bHeroicMode && m_auiEncounter[4] == DONE)
                        HandleGameObject(ObjectGuid::Empty, true, go);
                case 193208:
                    uiGalDarahDoor1 = go->GetGUID();
                    if (m_auiEncounter[3] == DONE)
                        HandleGameObject(ObjectGuid::Empty, true, go);
                    break;
                case 193209:
                    uiGalDarahDoor2 = go->GetGUID();
                    if (m_auiEncounter[3] == DONE)
                        HandleGameObject(ObjectGuid::Empty, true, go);
                    break;
                case 193188:
                    uiBridge = go->GetGUID();
                    go->SetGoState(uiBridgeState);
                    break;
                case 192633:
                    uiCollision = go->GetGUID();
                    go->SetGoState(uiCollisionState);

                    // Can't spawn here with SpawnGameObject because go isn't added to world yet...
                    if (uiCollisionState == GO_STATE_ACTIVE_ALTERNATIVE)
                        spawnSupport = true;
                    break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
            case DATA_SLAD_RAN_EVENT:
                m_auiEncounter[0] = data;
                if (data == DONE)
                {
                  GameObject* go = instance->GetGameObject(uiSladRanAltar);
                  if (go)
                      go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                }
                break;
            case DATA_MOORABI_EVENT:
                m_auiEncounter[1] = data;
                if (data == DONE)
                {
                  GameObject* go = instance->GetGameObject(uiMoorabiAltar);
                  if (go)
                      go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                  if (bHeroicMode)
                      HandleGameObject(uiEckTheFerociousDoor, true);
                }
                break;
            case DATA_DRAKKARI_COLOSSUS_EVENT:
                m_auiEncounter[2] = data;
                if (data == DONE)
                {
                  GameObject* go = instance->GetGameObject(uiDrakkariColossusAltar);
                  if (go)
                      go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                }
                break;
            case DATA_GAL_DARAH_EVENT:
                m_auiEncounter[3] = data;
                if (data == DONE)
                {
                    HandleGameObject(uiGalDarahDoor1, true);
                    HandleGameObject(uiGalDarahDoor2, true);
                }
                break;
            case DATA_ECK_THE_FEROCIOUS_EVENT:
                m_auiEncounter[4] = data;
                if (bHeroicMode && data == DONE)
                    HandleGameObject(uiEckTheFerociousDoorBehind, true);
                break;
            }

            if (data == DONE)
                SaveToDB();
        }

        void SetGuidData(uint32 type, ObjectGuid data) override
        {
            if (type == DATA_RUIN_DWELLER_DIED)
                DwellerGUIDs.erase(data);
            else if (type == DATA_STATUE_ACTIVATE)
            {
                toActivate = data;
                timer = 3500;
                ++phase;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_SLAD_RAN_EVENT:
                    return m_auiEncounter[0];
                case DATA_MOORABI_EVENT:
                    return m_auiEncounter[1];
                case DATA_GAL_DARAH_EVENT:
                    return m_auiEncounter[2];
                case DATA_DRAKKARI_COLOSSUS_EVENT:
                    return m_auiEncounter[3];
                case DATA_ECK_THE_FEROCIOUS_EVENT:
                    return m_auiEncounter[4];
                case DATA_ALIVE_RUIN_DWELLERS:
                    return DwellerGUIDs.size();
            }

            return 0;
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_SLAD_RAN_ALTAR:
                    return uiSladRanAltar;
                case DATA_MOORABI_ALTAR:
                    return uiMoorabiAltar;
                case DATA_DRAKKARI_COLOSSUS_ALTAR:
                    return uiDrakkariColossusAltar;
                case DATA_SLAD_RAN_STATUE:
                    return uiSladRanStatue;
                case DATA_MOORABI_STATUE:
                    return uiMoorabiStatue;
                case DATA_DRAKKARI_COLOSSUS_STATUE:
                    return uiDrakkariColossusStatue;
                case DATA_DRAKKARI_COLOSSUS:
                    return uiDrakkariColossus;
                case DATA_STATUE_ACTIVATE:
                    return toActivate;
            }

            return ObjectGuid::Empty;
        }

        std::string GetSaveData() override
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << "G D " << m_auiEncounter[0] << ' ' << m_auiEncounter[1] << ' '
                 << m_auiEncounter[2] << ' ' << m_auiEncounter[3] << ' ' << m_auiEncounter[4] << ' '
                 << (uiSladRanStatue ? GetObjState(uiSladRanStatue) : GO_STATE_ACTIVE) << ' ' << (uiMoorabiStatue ? GetObjState(uiMoorabiStatue) : GO_STATE_ACTIVE) << ' '
                 << (uiDrakkariColossusStatue ? GetObjState(uiDrakkariColossusStatue) : GO_STATE_ACTIVE) << ' ' << (uiGalDarahStatue ? GetObjState(uiGalDarahStatue) : GO_STATE_READY) << ' '
                 << (uiBridge ? GetObjState(uiBridge) : GO_STATE_ACTIVE) << ' ' << (uiCollision ? GetObjState(uiCollision) : GO_STATE_READY);

            str_data = saveStream.str();

            OUT_SAVE_INST_DATA_COMPLETE;
            return str_data;
        }

        void Load(const char* in) override
        {
            if (!in)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(in);

            char dataHead1, dataHead2;
            uint16 data0, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10;

            std::istringstream loadStream(in);
            loadStream >> dataHead1 >> dataHead2 >> data0 >> data1 >> data2 >> data3
                       >> data4 >> data5 >> data6 >> data7 >> data8 >> data9 >> data10;

            if (dataHead1 == 'G' && dataHead2 == 'D')
            {
                m_auiEncounter[0] = data0;
                m_auiEncounter[1] = data1;
                m_auiEncounter[2] = data2;
                m_auiEncounter[3] = data3;
                m_auiEncounter[4] = data4;
                uiSladRanStatueState            = GOState(data5);
                uiMoorabiStatueState            = GOState(data6);
                uiDrakkariColossusStatueState   = GOState(data7);
                uiGalDarahStatueState           = GOState(data8);
                uiBridgeState                   = GOState(data9);
                uiCollisionState                = GOState(data10);

                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                    if (m_auiEncounter[i] == IN_PROGRESS)
                        m_auiEncounter[i] = NOT_STARTED;
            } else OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }

         void Update(uint32 diff) override
         {
             // Spawn the support for the bridge if necessary
             if (spawnSupport)
             {
                 if (GameObject* pCollision = instance->GetGameObject(uiCollision))
                     pCollision->SummonGameObject(192743, pCollision->GetPositionX(), pCollision->GetPositionY(), pCollision->GetPositionZ(), pCollision->GetOrientation(), 0, 0, 0, 0, 0);
                 spawnSupport = false;
             }

             // If there is nothing to activate, then return
             if (!toActivate)
                 return;

             if (timer < diff)
             {
                 timer = 0;
                 if (toActivate == uiBridge)
                 {
                     GameObject* pBridge = instance->GetGameObject(uiBridge);
                     GameObject* pCollision = instance->GetGameObject(uiCollision);
                     GameObject* pSladRanStatue = instance->GetGameObject(uiSladRanStatue);
                     GameObject* pMoorabiStatue = instance->GetGameObject(uiMoorabiStatue);
                     GameObject* pDrakkariColossusStatue = instance->GetGameObject(uiDrakkariColossusStatue);
                     GameObject* pGalDarahStatue = instance->GetGameObject(uiGalDarahStatue);

                     toActivate.Clear();

                     if (pBridge && pCollision && pSladRanStatue && pMoorabiStatue && pDrakkariColossusStatue && pGalDarahStatue)
                     {
                         pBridge->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                         pCollision->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                         pSladRanStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                         pMoorabiStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                         pDrakkariColossusStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                         pGalDarahStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);

                         // Add the GO that solidifies the bridge so you can walk on it
                         spawnSupport = true;
                         SaveToDB();
                     }
                 }
                 else
                 {
                     uint32 spell = 0;
                     GameObject* pAltar = NULL;
                     if (toActivate == uiSladRanStatue)
                     {
                         spell = 57071;
                         pAltar = instance->GetGameObject(uiSladRanAltar);
                     }
                     else if (toActivate == uiMoorabiStatue)
                     {
                         spell = 57068;
                         pAltar = instance->GetGameObject(uiMoorabiAltar);
                     }
                     else if (toActivate == uiDrakkariColossusStatue)
                     {
                         spell = 57072;
                         pAltar = instance->GetGameObject(uiDrakkariColossusAltar);
                     }

                     // This is a workaround to make the beam cast properly. The caster should be ID 30298 but since the spells
                     // all are with scripted target for that same ID, it will hit itself.
                     if (pAltar)
                         if (Creature* trigger = pAltar->SummonCreature(18721, pAltar->GetPositionX(), pAltar->GetPositionY(), pAltar->GetPositionZ() + 3, pAltar->GetOrientation(), TEMPSUMMON_CORPSE_DESPAWN, 5000))
                         {
                             // Set the trigger model to invisible
                             trigger->SetDisplayId(11686);
                             trigger->CastSpell(trigger, spell, false);
                         }

                     if (GameObject* statueGO = instance->GetGameObject(toActivate))
                         statueGO->SetGoState(GO_STATE_READY);

                     toActivate.Clear();

                     if (phase == 3)
                         SetGuidData(DATA_STATUE_ACTIVATE, uiBridge);
                     else
                         SaveToDB(); // Don't save in between last statue and bridge turning in case of crash leading to stuck instance
                }
            }
            else
                timer -= diff;
        }

         GOState GetObjState(ObjectGuid guid)
         {
             if (GameObject* go = instance->GetGameObject(guid))
                 return go->GetGoState();
             return GO_STATE_ACTIVE;
         }
    };

};

class go_gundrak_altar : public GameObjectScript
{
public:
    go_gundrak_altar() : GameObjectScript("go_gundrak_altar") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go) override
    {
        InstanceScript* instance = go->GetInstanceScript();
        ObjectGuid uiStatue;

        go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
        go->SetGoState(GO_STATE_ACTIVE);

        if (instance)
        {
            switch (go->GetEntry())
            {
                case 192518:
                    uiStatue = instance->GetGuidData(DATA_SLAD_RAN_STATUE);
                    break;
                case 192519:
                    uiStatue = instance->GetGuidData(DATA_MOORABI_STATUE);
                    break;
                case 192520:
                    uiStatue = instance->GetGuidData(DATA_DRAKKARI_COLOSSUS_STATUE);
                    break;
            }

            if (!instance->GetGuidData(DATA_STATUE_ACTIVATE))
            {
                instance->SetGuidData(DATA_STATUE_ACTIVATE, uiStatue);
                go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                go->SetGoState(GO_STATE_ACTIVE);
            }
            return true;
        }
        return false;
    }

};

void AddSC_instance_gundrak()
{
    new instance_gundrak();
    new go_gundrak_altar();
}
