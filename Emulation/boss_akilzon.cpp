
#include "precompiled.h"
#include "zulaman.h"

enum
{
    SAY_AGGRO               = -1568026,
    SAY_SUMMON              = -1568027,
    SAY_SUMMON_ALT          = -1568028,
    SAY_ENRAGE              = -1568029,
    SAY_SLAY1               = -1568030,
    SAY_SLAY2               = -1568031,
    SAY_DEATH               = -1568032,
    EMOTE_STORM             = -1568033,

    SPELL_STATIC_DISRUPTION = 43622,
    SPELL_STATIC_VISUAL     = 45265,

    SPELL_CALL_LIGHTNING    = 43661,
    SPELL_GUST_OF_WIND      = 43621,

    SPELL_ELECTRICAL_STORM  = 43648,
    SPELL_STORMCLOUD_VISUAL = 45213,

    SPELL_BERSERK           = 45078,

    NPC_SOARING_EAGLE       = 24858,
    MAX_EAGLE_COUNT         = 6,

    //SE_LOC_X_MAX            = 400,
    //SE_LOC_X_MIN            = 335,
    //SE_LOC_Y_MAX            = 1435,
    //SE_LOC_Y_MIN            = 1370
};

struct MANGOS_DLL_DECL boss_akilzonAI : public ScriptedAI
{
    boss_akilzonAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiStaticDisruptTimer;
    uint32 m_uiCallLightTimer;
    uint32 m_uiGustOfWindTimer;
    uint32 m_uiStormTimer;
    uint32 m_uiSummonEagleTimer;
    uint32 m_uiBerserkTimer;
    bool m_bIsBerserk;

    void Reset()
    {
        m_uiStaticDisruptTimer = urand(7000, 14000);
        m_uiCallLightTimer = urand(15000, 25000);
        m_uiGustOfWindTimer = urand(20000, 30000);
        m_uiStormTimer = 50000;
        m_uiSummonEagleTimer = 65000;
        m_uiBerserkTimer = MINUTE*8*IN_MILLISECONDS;
        m_bIsBerserk = false;
    }

    void Aggro(Unit* pWho)
    {
        DoScriptText(SAY_AGGRO, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_AKILZON, IN_PROGRESS);
    }

    void KilledUnit(Unit* pVictim)
    {
        DoScriptText(urand(0, 1) ? SAY_SLAY1 : SAY_SLAY2, m_creature);
    }

    void JustDied(Unit* pKiller)
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (!m_pInstance)
            return;

        m_pInstance->SetData(TYPE_AKILZON, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_AKILZON, FAIL);
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_SOARING_EAGLE)
            pSummoned->SetInCombatWithZone();
    }

    void DoSummonEagles()
    {
        for(uint32 i = 0; i < MAX_EAGLE_COUNT; ++i)
        {
            float fX, fY, fZ;
            m_creature->GetRandomPoint(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ()+15.0f, 30.0f, fX, fY, fZ);

            if (Creature* pEagle = m_creature->SummonCreature(NPC_SOARING_EAGLE, fX, fY, m_creature->GetPositionZ() + frand(13.0f, 26.0f), m_creature->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 1000))
            {
                pEagle->SetWalk(false);
                pEagle->SetLevitate(true);
                pEagle->CastSpell(pEagle, 17131, true);
            }
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiCallLightTimer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                DoCast(m_creature->getVictim(), SPELL_CALL_LIGHTNING, true);
                m_uiCallLightTimer = urand(15000, 25000);
            }
            else
                m_uiCallLightTimer += 5000;
        }
        else
            m_uiCallLightTimer -= uiDiff;

        if (m_uiStaticDisruptTimer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))
                    DoCast(pTarget, SPELL_STATIC_DISRUPTION, true);

                m_uiStaticDisruptTimer = urand(7000, 14000);
            }
            else
                m_uiStaticDisruptTimer += 5000;
        }
        else 
            m_uiStaticDisruptTimer -= uiDiff;

        if (m_uiStormTimer < uiDiff)
        {
            if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
            {
                if (m_creature->IsNonMeleeSpellCasted(false))
                    m_creature->InterruptNonMeleeSpells(false);

                DoScriptText(EMOTE_STORM, m_creature);
                DoCast(pTarget, SPELL_ELECTRICAL_STORM, false);
            }

            m_uiStormTimer = 60000;
        }
        else
            m_uiStormTimer -= uiDiff;

        if (m_uiGustOfWindTimer < uiDiff)
        {
            if (!m_creature->IsNonMeleeSpellCasted(false))
            {
                if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))
                    DoCast(pTarget, SPELL_GUST_OF_WIND, true);

                m_uiGustOfWindTimer = urand(20000, 30000);
            }
            else
                m_uiGustOfWindTimer += 5000;
        }
        else
            m_uiGustOfWindTimer -= uiDiff;

        if (m_uiSummonEagleTimer < uiDiff)
        {
            DoScriptText(urand(0,1) ? SAY_SUMMON : SAY_SUMMON_ALT, m_creature);
            DoSummonEagles();
            m_uiSummonEagleTimer = m_uiStormTimer + 15000;
        }
        else
            m_uiSummonEagleTimer -= uiDiff;

        if (!m_bIsBerserk && m_uiBerserkTimer < uiDiff)
        {
            DoScriptText(SAY_ENRAGE, m_creature);
            DoCast(m_creature, SPELL_BERSERK, true);
            m_bIsBerserk = true;
        }
        else
            m_uiBerserkTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_akilzon(Creature* pCreature)
{
    return new boss_akilzonAI(pCreature);
}

enum
{
    SPELL_EAGLE_SWOOP       = 44732,
    POINT_ID_RANDOM         = 1
};

const float minEagleZ = 84.f;
const float maxEagleZ = 98.f;

struct MANGOS_DLL_DECL mob_soaring_eagleAI : public ScriptedAI
{
    mob_soaring_eagleAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
        CCW = urand(0,1);
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiEagleSwoopTimer;
    uint32 m_uiReturnTimer;
    uint32 m_uiArcRadius;
    uint32 m_uiArcTimer;
    uint32 m_uiMovementInterval;
    float x, y, z;
    bool m_bCanCast;
    bool CCW;

    void Reset()
    {
        m_uiEagleSwoopTimer = urand(5000,7000);
        m_uiReturnTimer = 800;
        m_uiArcRadius = 20;
        m_uiArcTimer = 0;
        m_uiMovementInterval = 200;
        m_bCanCast = true;

        m_creature->GetPosition(x, y, z);
    }

    void AttackStart(Unit* pWho)
    {
        if (!pWho)
            return;

        if (m_creature->Attack(pWho, false))
        {
            m_creature->AddThreat(pWho);
            m_creature->SetInCombatWith(pWho);
            pWho->SetInCombatWith(m_creature);
        }
    }

    void DoMoveEagleCircle()
    {
        float newX, newY, newZ;

        newX = x + m_uiArcRadius*cos(m_uiArcTimer*M_PI_F/12);
        newY = CCW ? y + m_uiArcRadius*sin(m_uiArcTimer*M_PI_F/12) : y - m_uiArcRadius*sin(m_uiArcTimer*M_PI_F/12);

        float curZ = m_creature->GetPositionZ();

        if (curZ < minEagleZ)
        {
            newZ = minEagleZ + frand(0.1f, 2.2f);
        }
        else if (curZ > maxEagleZ)
        {
            newZ = maxEagleZ - frand(1.0f, 3.3f);
        }
        else
            newZ = urand(0,1) ? curZ + frand(1.2f, 2.25f) : curZ - frand(1.2f, 2.25f);

        m_creature->SetWalk(false);
        m_creature->GetMotionMaster()->MovementExpired();
        m_creature->GetMotionMaster()->MovePoint(0, newX, newY, newZ);
    }

    void MovementInform(uint32 uiType, uint32 uiPointId)
    {
        if (uiType != POINT_MOTION_TYPE)
            return;

        m_bCanCast = true;
        ++m_uiArcTimer;
        m_uiMovementInterval = 100;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiMovementInterval)
        {
            if (m_uiMovementInterval <= uiDiff)
            {
                DoMoveEagleCircle();
                m_uiMovementInterval = 0;
            }
            else
                m_uiMovementInterval -= uiDiff;
        }

        if (!m_bCanCast)
            return;

        if (m_uiEagleSwoopTimer < uiDiff)
        {
            if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
            {
                DoCastSpellIfCan(pTarget,SPELL_EAGLE_SWOOP);
                DoPlaySoundToSet(m_creature, 3330);

                m_bCanCast = false;
                m_uiMovementInterval = 1700;
            }

            m_uiEagleSwoopTimer = urand(4000, 6000);
        }
        else 
            m_uiEagleSwoopTimer -= uiDiff;
    }
};

CreatureAI* GetAI_mob_soaring_eagle(Creature* pCreature)
{
    return new mob_soaring_eagleAI(pCreature);
}

void AddSC_boss_akilzon()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_akilzon";
    pNewScript->GetAI = &GetAI_boss_akilzon;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "mob_soaring_eagle";
    pNewScript->GetAI = &GetAI_mob_soaring_eagle;
    pNewScript->RegisterSelf();
}
