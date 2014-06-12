/* Copyright (C) 2006 - 2011 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Mother_Shahraz
SD%Complete: 80
SDComment: Fatal Attraction slightly incorrect; need to damage only if affected players are within range of each other
SDCategory: Black Temple
EndScriptData */

#include "precompiled.h"
#include "black_temple.h"

enum
{
	//Speech'n'Sounds
	SAY_TAUNT1         = -1564018,
	SAY_TAUNT2         = -1564019,
	SAY_TAUNT3         = -1564020,
	SAY_AGGRO          = -1564021,
	SAY_SPELL1         = -1564022,
	SAY_SPELL2         = -1564023,
	SAY_SPELL3         = -1564024,
	SAY_SLAY1          = -1564025,
	SAY_SLAY2          = -1564026,
	SAY_ENRAGE         = -1564027,
	SAY_DEATH          = -1564028,

	//Spells
	SPELL_BEAM_SINISTER			= 40859,
	SPELL_BEAM_VILE				= 40860,
	SPELL_BEAM_WICKED			= 40861,
	SPELL_BEAM_SINFUL			= 40827,
	SPELL_ATTRACTION			= 40871,
	SPELL_SILENCING_SHRIEK		= 40823,
	SPELL_ENRAGE				= 23537,
	SPELL_TELEPORT_VISUAL		= 41232, // Using 40869 would require hackfixes all over the place
	SPELL_BERSERK				= 45078,
	SPELL_FATAL_ATTRACTION_AURA = 41001,
	SPELL_SABER_LASH_AURA       = 40816,
    SPELL_SABER_LASH            = 43690,
};

struct AuraData
{
    uint32 mAuraId;
    int32 mCastAmount;
    int32 mSchoolDamage;
};

const uint32 AurasIds[]=
{
    0,                                                      // Normal
    40897,                                                  // Holy
    40882,                                                  // Fire
    40883,                                                  // Nature
    40896,                                                  // Frost
    40880,                                                  // Shadow
    40891,                                                  // Arcane
};

struct Locations
{
    float x,y,z;
};

const uint32 auiBeamSpells[4] =
{
	{ SPELL_BEAM_SINISTER },
	{ SPELL_BEAM_WICKED },
	{ SPELL_BEAM_VILE },
	{ SPELL_BEAM_SINFUL }
};

static Locations TeleportPoint[]=
{
    {959.996f, 212.576f, 193.843f},
    {932.537f, 231.813f, 193.838f},
    {958.675f, 254.767f, 193.822f},
    {946.955f, 201.316f, 192.535f},
    {944.294f, 149.676f, 197.551f},
    {930.548f, 284.888f, 193.367f},
    {965.997f, 278.398f, 195.777f}
};

struct MANGOS_DLL_DECL boss_shahrazAI : public ScriptedAI
{
    boss_shahrazAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    ObjectGuid m_targetGuid[3];
    uint32 m_uiBeamTimer;
    uint32 m_uiBeamCount;
    uint32 m_uiCurrentBeam;
    uint32 m_uiPrismaticShieldTimer;
    uint32 m_uiFatalAttractionTimer;
    uint32 m_uiFatalAttractionExplodeTimer;
    uint32 m_uiShriekTimer;
    uint32 m_uiRandomYellTimer;
    uint32 m_uiEnrageTimer;
    uint32 m_uiExplosionCount;
	uint32 m_uiCheckTimer;

	GuidList m_list;

    std::map<uint32/*school*/, AuraData*> auraMap;

    bool m_bEnraged;

    void Reset()
    {
        ResetAuraData();

        for (int i = 0; i < 3; ++i)
            m_targetGuid[i].Clear();

        m_uiBeamTimer = 9000;                                  // Timers may be incorrect
        m_uiBeamCount = 0;
        m_uiCurrentBeam = 0;                                    // 0 - Sinister, 1 - Vile, 2 - Wicked, 3 - Sinful
        m_uiPrismaticShieldTimer = 15000;
        m_uiFatalAttractionTimer = 25000;
        m_uiFatalAttractionExplodeTimer = 70000;
        m_uiShriekTimer = 30000;
        m_uiRandomYellTimer = urand(30000,60000);
        m_uiEnrageTimer = 600000;
        m_uiExplosionCount = 0;

        m_bEnraged = false;

		// This aura procs the saber lash cleave and immunity automatically
		if (!m_creature->HasAura(SPELL_SABER_LASH_AURA))
			DoCast(m_creature, SPELL_SABER_LASH_AURA, true);
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SHAHRAZ, IN_PROGRESS);

		if (!m_creature->HasAura(SPELL_SABER_LASH_AURA))
			DoCastSpellIfCan(m_creature, SPELL_SABER_LASH_AURA, CAST_TRIGGERED);

        DoScriptText(SAY_AGGRO, m_creature);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SHAHRAZ, NOT_STARTED);

		m_creature->RemoveAurasDueToSpell(SPELL_BERSERK);
    }

	bool IsInList(ObjectGuid Guid)
	{
		if (!m_list.empty())
		{
		    for (GuidList::const_iterator itr = m_list.begin(); itr != m_list.end(); ++itr)
		    {
		    	if (Unit* pUnit = m_creature->GetMap()->GetUnit(*itr))
		    	{
			    	ObjectGuid mGuid = pUnit->GetObjectGuid();

					if (mGuid == Guid)
						return true;
				}
			}
		}

		return false;
	}

	bool IsInSet(ObjectGuid Guid, GuidSet mSet)
	{
		if (!mSet.empty())
		{
		    for (GuidSet::const_iterator itr = mSet.begin(); itr != mSet.end(); ++itr)
		    {
		    	if (Unit* pUnit = m_creature->GetMap()->GetUnit(*itr))
		    	{
			    	ObjectGuid mGuid = pUnit->GetObjectGuid();

					if (mGuid == Guid)
						return true;
				}
			}
		}

		return false;
	}

    void ResetAuraData()
    {
        auraMap.clear();

        // Meh, we could just do this the first time and clear after
        // Extra code is nonsense though, and this won't do any harm to make sure it's fersh
        for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        {
            AuraData* aur = new AuraData();
            aur->mAuraId = AurasIds[i];
            aur->mCastAmount = 0;
            auraMap[i] = aur;
        }
    }

    void AdjustAuraData(uint32 school, uint32 damage)
    {
        std::map<uint32, AuraData*>::const_iterator itr = auraMap.find(school);
        if( itr != auraMap.cend() )
        {
            itr->second->mSchoolDamage += damage;
			itr->second->mCastAmount = (itr->second->mSchoolDamage / 4000);

            if (itr->second->mCastAmount > 9)
                itr->second->mCastAmount = 9;
        }
        else //This won't happen, but it's called safety!
            printf("ERROR: AuraData is missing for school %i\n", school);
    }

	void HandleAuraApplication()
	{
        for (std::map<uint32, AuraData*>::const_iterator itr = auraMap.begin(); itr != auraMap.end(); ++itr)
        {
            if (itr->second->mCastAmount == 0)
                continue;

            int castAmount = itr->second->mCastAmount;
            for (int i = 0; i < castAmount; ++i)
                DoCast(m_creature, itr->second->mAuraId, true);
        }
	}

    void KilledUnit(Unit *victim)
    {
        DoScriptText(urand(0, 1) ? SAY_SLAY1 : SAY_SLAY2, m_creature);
    }

    void JustDied(Unit *victim)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SHAHRAZ, DONE);

        DoScriptText(SAY_DEATH, m_creature);
    }

	// This should easily be doable using the normal spell_target_position table with coordinates instead of this hackfix
    void TeleportPlayers()
	{
		std::vector<Unit*> vTargets;
		ThreatList const& tList = m_creature->getThreatManager().getThreatList();
		ThreatList::const_iterator itr = tList.begin();

		for (; itr != tList.end(); itr++)
        {
			// Add only alive players to the target list
			if (Unit* pTarget = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
                if (pTarget->GetTypeId() == TYPEID_PLAYER && pTarget->isAlive() && !pTarget->HasAura(SPELL_SABER_LASH))
					vTargets.push_back(pTarget);
        }

		if (vTargets.empty())
			return;

		// Shrink down to 3 targets in any case
		while (vTargets.size() > 3)
			vTargets.erase(vTargets.begin() + urand(0, vTargets.size() - 1));
		
		uint8 uiRand = urand(0, 6);

		for (std::vector<Unit*>::const_iterator iter = vTargets.begin(); iter != vTargets.end(); ++iter)
		{
			if (Unit* pPlayer = *iter)
			{
				pPlayer->CastSpell(pPlayer, SPELL_TELEPORT_VISUAL, true);
				DoTeleportPlayer(pPlayer, TeleportPoint[uiRand].x, TeleportPoint[uiRand].y, TeleportPoint[uiRand].z, pPlayer->GetOrientation());
				pPlayer->CastSpell(pPlayer, SPELL_FATAL_ATTRACTION_AURA, true);
			}
		}
	}
			
	void DamageTaken(Unit* pWho, uint32& uiDamage, const SpellEntry* pSpell)
	{
		if (uiDamage && pSpell)
		{
			uint32 school = GetFirstSchoolInMask( SpellSchoolMask(pSpell->schoolMask) );
			AdjustAuraData(school, uiDamage);

			debug_log("Mother Took school damage from Player %s, School %u", pWho->GetObjectGuid().GetString().c_str(), school);
		}
	}

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

		if (m_uiCheckTimer <= uiDiff)
		{
			CheckAura(SPELL_SABER_LASH_AURA, true);
			m_uiCheckTimer = 2000;
		}
		else
			m_uiCheckTimer -= uiDiff;

        if (m_creature->GetHealthPercent() < 10.0f && !m_bEnraged)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE, CAST_TRIGGERED) == CAST_OK)
			{
				m_bEnraged = true;
				DoScriptText(SAY_ENRAGE, m_creature);
			}
        }

        //Randomly cast one beam.
        if (m_uiBeamTimer < uiDiff)
        {
            Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
            if (!pTarget || !pTarget->isAlive())
                return;

            if (DoCastSpellIfCan(pTarget, auiBeamSpells[m_uiCurrentBeam]) == CAST_OK)
			{
				++m_uiBeamCount;

				if (m_uiBeamCount > 4)
				{
					m_uiCurrentBeam = (m_uiCurrentBeam + urand(1, 3)) % 4;
					m_uiBeamCount = 0;
				}

				m_uiBeamTimer = 9000;
			}
        }
		else 
			m_uiBeamTimer -= uiDiff;

        // Apply prismatic aura to players in accordance to damage taken
        if (m_uiPrismaticShieldTimer < uiDiff)
        {
            HandleAuraApplication();
            m_uiPrismaticShieldTimer = 15000;
			ResetAuraData();
        }
		else 
			m_uiPrismaticShieldTimer -= uiDiff;

        // Select 3 random targets (can select same target more than once), teleport to a random location then make them cast explosions until they get away from each other.
        if (m_uiFatalAttractionTimer < uiDiff)
        {
            m_uiExplosionCount = 0;

            TeleportPlayers();

            DoScriptText(urand(0, 1) ? SAY_SPELL2 : SAY_SPELL3, m_creature);

            m_uiFatalAttractionExplodeTimer = 2000;
            m_uiFatalAttractionTimer = urand(40000, 70000);
        }
		else
			m_uiFatalAttractionTimer -= uiDiff;

        if (m_uiShriekTimer < uiDiff)
        {
            DoCast(m_creature->getVictim(), SPELL_SILENCING_SHRIEK);
            m_uiShriekTimer = 30000;
        }
		else 
			m_uiShriekTimer -= uiDiff;

        //Enrage
        if (!m_creature->HasAura(SPELL_BERSERK, EFFECT_INDEX_0))
        {
            if (m_uiEnrageTimer < uiDiff)
            {
                DoCast(m_creature, SPELL_BERSERK);
                DoScriptText(SAY_ENRAGE, m_creature);
            }
			else 
				m_uiEnrageTimer -= uiDiff;
        }

        //Random taunts
        if (m_uiRandomYellTimer < uiDiff)
        {
            switch(urand(0, 2))
            {
                case 0: DoScriptText(SAY_TAUNT1, m_creature); break;
                case 1: DoScriptText(SAY_TAUNT2, m_creature); break;
                case 2: DoScriptText(SAY_TAUNT3, m_creature); break;
            }

            m_uiRandomYellTimer = urand(60000, 150000);
        }
		else 
			m_uiRandomYellTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_shahraz(Creature* pCreature)
{
    return new boss_shahrazAI(pCreature);
}

void AddSC_boss_mother_shahraz()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_mother_shahraz";
    pNewScript->GetAI = &GetAI_boss_shahraz;
    pNewScript->RegisterSelf();
}
