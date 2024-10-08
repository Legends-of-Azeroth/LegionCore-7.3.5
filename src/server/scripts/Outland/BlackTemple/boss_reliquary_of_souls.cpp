/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
#include "ScriptedCreature.h"
#include "black_temple.h"
#include "Spell.h"

//Sound'n'speech
enum Says
{
    //Suffering
    SUFF_SAY_FREED = 0,
    SUFF_SAY_AGGRO,
    SUFF_SAY_SLAY,
    SUFF_SAY_RECAP,
    SUFF_EMOTE_ENRAGE,

    //Desire
    DESI_SAY_FREED = 0,
    DESI_SAY_SLAY,
    DESI_SAY_SPEC,
    DESI_SAY_RECAP,

    //Anger
    ANGER_SAY_FREED = 0,
    ANGER_SAY_SLAY,
    ANGER_SAY_SPEC,
    ANGER_SAY_BEFORE,
    ANGER_SAY_DEATH,

    SUFF_SAY_AFTER = 0,
    DESI_SAY_AFTER
};

//Spells
#define AURA_OF_SUFFERING               41292
#define AURA_OF_SUFFERING_ARMOR         42017 // linked aura, need core support
#define ESSENCE_OF_SUFFERING_PASSIVE    41296 // periodic trigger 41294
#define ESSENCE_OF_SUFFERING_PASSIVE2   41623
#define SPELL_FIXATE_TARGET             41294 // dummy, select target
#define SPELL_FIXATE_TAUNT              41295 // force taunt
#define SPELL_ENRAGE                    41305
#define SPELL_SOUL_DRAIN                41303

#define AURA_OF_DESIRE                  41350
#define AURA_OF_DESIRE_DAMAGE           41352
#define SPELL_RUNE_SHIELD               41431
#define SPELL_DEADEN                    41410
#define SPELL_SOUL_SHOCK                41426

#define AURA_OF_ANGER                   41337
#define SPELL_SELF_SEETHE               41364 // force cast 41520
#define SPELL_ENEMY_SEETHE              41520
#define SPELL_SOUL_SCREAM               41545
#define SPELL_SPITE_TARGET              41376 // cast 41377 after 6 sec
#define SPELL_SPITE_DAMAGE              41377

#define ENSLAVED_SOUL_PASSIVE           41535
#define SPELL_SOUL_RELEASE              41542
#define SPELL_SUBMERGE                  37550 //dropout 'head'

#define CREATURE_ENSLAVED_SOUL          23469
#define NUMBER_ENSLAVED_SOUL            8

struct Position2d
{
    float x, y;
};

static Position2d Coords[]=
{
    {450.4f, 212.3f},
    {542.1f, 212.3f},
    {542.1f, 168.3f},
    {542.1f, 137.4f},
    {450.4f, 137.4f},
    {450.4f, 168.3f}
};

class npc_enslaved_soul : public CreatureScript
{
public:
    npc_enslaved_soul() : CreatureScript("npc_enslaved_soul") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_enslaved_soulAI (creature);
    }

    struct npc_enslaved_soulAI : public ScriptedAI
    {
        npc_enslaved_soulAI(Creature* creature) : ScriptedAI(creature) {}

        ObjectGuid ReliquaryGUID;

        void Reset() {ReliquaryGUID.Clear();}

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(me, ENSLAVED_SOUL_PASSIVE, true);
            DoZoneInCombat();
        }

        void JustDied(Unit* /*killer*/);
    };

};

class boss_reliquary_of_souls : public CreatureScript
{
public:
    boss_reliquary_of_souls() : CreatureScript("boss_reliquary_of_souls") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_reliquary_of_soulsAI (creature);
    }

    struct boss_reliquary_of_soulsAI : public ScriptedAI
    {
        boss_reliquary_of_soulsAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            EssenceGUID.Clear();
        }

        InstanceScript* instance;

        ObjectGuid EssenceGUID;

        uint32 Phase;
        uint32 Counter;
        uint32 Timer;

        uint32 SoulCount;
        uint32 SoulDeathCount;

        void Reset()
        {
            if (instance)
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, NOT_STARTED);

            if (EssenceGUID)
            {
                if (Creature* Essence = Unit::GetCreature(*me, EssenceGUID))
                {
                    Essence->DespawnOrUnsummon();
                }
                EssenceGUID.Clear();
            }

            Phase = 0;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_ONESHOT_NONE);
            me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
        }

        void MoveInLineOfSight(Unit* who)

        {
            if (!who)
                return;
            if (me->isInCombat())
                return;
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;
            if (me->GetDistance(who) > 50.0f)
                return;
            AttackStartNoMove(who);
        }

        void EnterCombat(Unit* who)
        {
            me->AddThreat(who, 10000.0f);
            DoZoneInCombat();
            if (instance)
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, IN_PROGRESS);

            Phase = 1;
            Counter = 0;
            Timer = 0;
        }

        bool SummonSoul()
        {
            uint32 random = rand()%6;
            float x = Coords[random].x;
            float y = Coords[random].y;

            Creature* Soul = me->SummonCreature(CREATURE_ENSLAVED_SOUL, x, y, me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_CORPSE_DESPAWN, 0);
            if (!Soul)
                return false;

            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
            {
                CAST_AI(npc_enslaved_soul::npc_enslaved_soulAI, Soul->AI())->ReliquaryGUID = me->GetGUID();
                Soul->AI()->AttackStart(target);
            } else EnterEvadeMode();
            return true;
        }

        void MergeThreatList(Creature* target)
        {
            if (!target)
                return;

            std::list<HostileReference*>& m_threatlist = target->getThreatManager().getThreatList();
            std::list<HostileReference*>::const_iterator itr = m_threatlist.begin();
            for (; itr != m_threatlist.end(); ++itr)
            {
                Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid());
                if (unit)
                {
                    DoModifyThreatPercent(unit, -100);
                    float threat = target->getThreatManager().getThreat(unit);
                    me->AddThreat(unit, threat);       // This makes it so that the unit has the same amount of threat in Reliquary's threatlist as in the target creature's (One of the Essences).
                }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, DONE);
        }

        void UpdateAI(uint32 diff)
        {
            if (!Phase)
                return;

            if (me->getThreatManager().getThreatList().empty()) // Reset if event is begun and we don't have a threatlist
            {
                EnterEvadeMode();
                return;
            }

            Creature* Essence = NULL;
            if (EssenceGUID)
            {
                Essence = Unit::GetCreature(*me, EssenceGUID);
                if (!Essence)
                {
                    EnterEvadeMode();
                    return;
                }
            }

            if (Timer <= diff)
            {
                switch (Counter)
                {
                case 0:
                    me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_STATE_READY2H);  // I R ANNNGRRRY!
                    DoStartNoMovement(me);
                    Timer = 3000;
                    break;
                case 1:
                    Timer = 2800;
                    me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_ONESHOT_SUBMERGE);  // Release the cube
                    DoCast(me, SPELL_SUBMERGE);
                    DoStartNoMovement(me);
                    break;
                case 2:
                    Timer = 5000;
                    if (Creature* Summon = DoSpawnCreature(23417+Phase, 0, 0, 0, 0, TEMPSUMMON_DEAD_DESPAWN, 0))
                    {
                        me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_STATE_SUBMERGED);  // Ribs: open
                        Summon->AI()->AttackStart(SelectTarget(SELECT_TARGET_TOPAGGRO, 0));
                        EssenceGUID = Summon->GetGUID();
                        DoStartNoMovement(me);
                    } else EnterEvadeMode();
                    break;
                case 3:
                    Timer = 1000;
                    if (Phase == 3)
                    {
                        if (!Essence->IsAlive())
                            DoCast(me, 7, true);
                        else return;
                    }
                    else
                    {
                        if (Essence->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                        {
                            MergeThreatList(Essence);
                            Essence->RemoveAllAuras();
                            Essence->DeleteThreatList();
                            Essence->GetMotionMaster()->MoveFollow(me, 0.0f, 0.0f);
                        } else return;
                    }
                    break;
                case 4:
                    Timer = 1500;
                    if (Essence->IsWithinDistInMap(me, 10))
                    {
                        Essence->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, 374); //rotate and disappear
                        Timer = 2000;
                        me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                    }
                    else
                    {
                        MergeThreatList(Essence);
                        Essence->RemoveAllAuras();
                        Essence->DeleteThreatList();
                        Essence->GetMotionMaster()->MoveFollow(me, 0, 0);
                        return;
                    }
                    break;
                case 5:
                    if (Phase == 1)
                        Essence->AI()->Talk(SUFF_SAY_AFTER);
                    else
                        Essence->AI()->Talk(DESI_SAY_AFTER);
                    Essence->DespawnOrUnsummon();
                    me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, 0);
                    EssenceGUID.Clear();
                    SoulCount = 0;
                    SoulDeathCount = 0;
                    Timer = 3000;
                    break;
                case 6:
                    if (SoulCount < NUMBER_ENSLAVED_SOUL)
                    {
                        if (SummonSoul())
                            ++SoulCount;
                        Timer = 500;
                        return;
                    }
                    break;
                case 7:
                    if (SoulDeathCount >= SoulCount)
                    {
                        Counter = 1;
                        ++Phase;
                        Timer = 5000;
                    }
                    return;
                default:
                    break;
                }
                ++Counter;
            } else Timer -= diff;
        }
    };

};

void npc_enslaved_soul::npc_enslaved_soulAI::JustDied(Unit* /*killer*/)
{
    if (ReliquaryGUID)
        if (Creature* Reliquary = (Unit::GetCreature((*me), ReliquaryGUID)))
            ++(CAST_AI(boss_reliquary_of_souls::boss_reliquary_of_soulsAI, Reliquary->AI())->SoulDeathCount);

    DoCast(me, SPELL_SOUL_RELEASE, true);
}

class boss_essence_of_suffering : public CreatureScript
{
public:
    boss_essence_of_suffering() : CreatureScript("boss_essence_of_suffering") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_essence_of_sufferingAI (creature);
    }

    struct boss_essence_of_sufferingAI : public ScriptedAI
    {
        boss_essence_of_sufferingAI(Creature* creature) : ScriptedAI(creature) {}

        ObjectGuid StatAuraGUID;

        uint32 AggroYellTimer;
        uint32 FixateTimer;
        uint32 EnrageTimer;
        uint32 SoulDrainTimer;
        uint32 AuraTimer;

        void Reset()
        {
            StatAuraGUID.Clear();

            AggroYellTimer = 5000;
            FixateTimer = 8000;
            EnrageTimer = 30000;
            SoulDrainTimer = 45000;
            AuraTimer = 5000;
        }

        void DamageTaken(Unit* /*done_by*/, uint32 &damage, DamageEffectType dmgType)
        {
            if (damage >= me->GetHealth())
            {
                damage = 0;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                Talk(SUFF_SAY_RECAP);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
            {
                Talk(SUFF_SAY_FREED);
                DoZoneInCombat();
                DoCast(me, AURA_OF_SUFFERING, true); // linked aura need core support
                DoCast(me, ESSENCE_OF_SUFFERING_PASSIVE, true);
                DoCast(me, ESSENCE_OF_SUFFERING_PASSIVE2, true);
            }
            else return;
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SUFF_SAY_SLAY);
        }

        void CastFixate()
        {
            std::list<HostileReference*>& m_threatlist = me->getThreatManager().getThreatList();
            if (m_threatlist.empty())
                return; // No point continuing if empty threatlist.
            std::list<Unit*> targets;
            std::list<HostileReference*>::const_iterator itr = m_threatlist.begin();
            for (; itr != m_threatlist.end(); ++itr)
            {
                Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid());
                if (unit && unit->IsAlive() && (unit->GetTypeId() == TYPEID_PLAYER)) // Only alive players
                    targets.push_back(unit);
            }
            if (targets.empty())
                return; // No targets added for some reason. No point continuing.
            targets.sort(Trinity::ObjectDistanceOrderPred(me)); // Sort players by distance.
            targets.resize(1); // Only need closest target.
            Unit* target = targets.front(); // Get the first target.
            if (target)
                target->CastSpell(me, SPELL_FIXATE_TAUNT, true);
            DoResetThreat();
            me->AddThreat(target, 1000000);
        }

        void UpdateAI(uint32 diff)
        {
            if (me->isInCombat())
            {
                //Supposed to be cast on nearest target
                if (FixateTimer <= diff)
                {
                    CastFixate();
                    FixateTimer = 5000;
                    if (!(rand()%16))
                        Talk(SUFF_SAY_AGGRO);
                } else FixateTimer -= diff;
            }

            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (EnrageTimer <= diff)
            {
                DoCast(me, SPELL_ENRAGE);
                EnrageTimer = 60000;
                Talk(SUFF_EMOTE_ENRAGE);
            } else EnrageTimer -= diff;

            if (SoulDrainTimer <= diff)
            {
                DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0), SPELL_SOUL_DRAIN);
                SoulDrainTimer = 60000;
            } else SoulDrainTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

};

class boss_essence_of_desire : public CreatureScript
{
public:
    boss_essence_of_desire() : CreatureScript("boss_essence_of_desire") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_essence_of_desireAI (creature);
    }

    struct boss_essence_of_desireAI : public ScriptedAI
    {
        boss_essence_of_desireAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 RuneShieldTimer;
        uint32 DeadenTimer;
        uint32 SoulShockTimer;

        void Reset()
        {
            RuneShieldTimer = 60000;
            DeadenTimer = 30000;
            SoulShockTimer = 5000;
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);
        }

        void DamageTaken(Unit* done_by, uint32 &damage, DamageEffectType dmgType)
        {
            if (done_by == me)
                return;

            if (damage >= me->GetHealth())
            {
                damage = 0;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                Talk(SUFF_SAY_RECAP);
                me->SetReactState(REACT_PASSIVE);
            }
            else
            {
                float bp0 = damage / 2;
                me->CastCustomSpell(done_by, AURA_OF_DESIRE_DAMAGE, &bp0, NULL, NULL, true);
            }
        }

        void SpellHit(Unit* /*caster*/, const SpellInfo* spell)
        {
            if (me->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                for (uint8 i = 0; i < 3; ++i)
                    if (spell->Effects[i]->Effect == SPELL_EFFECT_INTERRUPT_CAST)
                        if (me->GetCurrentSpell(CURRENT_GENERIC_SPELL)->m_spellInfo->Id == SPELL_SOUL_SHOCK
                            || me->GetCurrentSpell(CURRENT_GENERIC_SPELL)->m_spellInfo->Id == SPELL_DEADEN)
                            me->InterruptSpell(CURRENT_GENERIC_SPELL, false);
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(DESI_SAY_FREED);
            DoZoneInCombat();
            DoCast(me, AURA_OF_DESIRE, true);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(DESI_SAY_SLAY);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (RuneShieldTimer <= diff)
            {
                me->InterruptNonMeleeSpells(false);
                DoCast(me, SPELL_RUNE_SHIELD, true);
                SoulShockTimer += 2000;
                DeadenTimer += 2000;
                RuneShieldTimer = 60000;
            } else RuneShieldTimer -= diff;

            if (SoulShockTimer <= diff)
            {
                DoCast(me->getVictim(), SPELL_SOUL_SHOCK);
                SoulShockTimer = 5000;
            } else SoulShockTimer -= diff;

            if (DeadenTimer <= diff)
            {
                me->InterruptNonMeleeSpells(false);
                DoCast(me->getVictim(), SPELL_DEADEN);
                DeadenTimer = urand(25000, 35000);
                if (!(rand()%2))
                    Talk(DESI_SAY_SPEC);
            } else DeadenTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

};

class boss_essence_of_anger : public CreatureScript
{
public:
    boss_essence_of_anger() : CreatureScript("boss_essence_of_anger") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_essence_of_angerAI (creature);
    }

    struct boss_essence_of_angerAI : public ScriptedAI
    {
        boss_essence_of_angerAI(Creature* creature) : ScriptedAI(creature) {}

        ObjectGuid AggroTargetGUID;

        uint32 CheckTankTimer;
        uint32 SoulScreamTimer;
        uint32 SpiteTimer;

        GuidList SpiteTargetGUID;

        bool CheckedAggro;

        void Reset()
        {
            AggroTargetGUID.Clear();

            CheckTankTimer = 5000;
            SoulScreamTimer = 10000;
            SpiteTimer = 30000;

            SpiteTargetGUID.clear();

            CheckedAggro = false;
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(ANGER_SAY_FREED);

            DoZoneInCombat();
            DoCast(me, AURA_OF_ANGER, true);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(ANGER_SAY_DEATH);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(ANGER_SAY_SLAY);
        }

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (!CheckedAggro)
            {
                AggroTargetGUID = me->getVictim()->GetGUID();
                CheckedAggro = true;
            }

            if (CheckTankTimer <= diff)
            {
                if (me->getVictim()->GetGUID() != AggroTargetGUID)
                {
                    Talk(ANGER_SAY_BEFORE);
                    DoCast(me, SPELL_SELF_SEETHE, true);
                    AggroTargetGUID = me->getVictim()->GetGUID();
                }
                CheckTankTimer = 2000;
            } else CheckTankTimer -= diff;

            if (SoulScreamTimer <= diff)
            {
                DoCast(me->getVictim(), SPELL_SOUL_SCREAM);
                SoulScreamTimer = urand(9000, 11000);
                if (!(rand()%3))
                    Talk(ANGER_SAY_SPEC);
            } else SoulScreamTimer -= diff;

            if (SpiteTimer <= diff)
            {
                DoCast(me, SPELL_SPITE_TARGET);
                SpiteTimer = 30000;
                Talk(ANGER_SAY_SPEC);
            } else SpiteTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

};

void AddSC_boss_reliquary_of_souls()
{
    new boss_reliquary_of_souls();
    new boss_essence_of_suffering();
    new boss_essence_of_desire();
    new boss_essence_of_anger();
    new npc_enslaved_soul();
}
