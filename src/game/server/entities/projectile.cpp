/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "projectile.h"

#include <engine/shared/config.h>
#include <game/server/teams.h>

CProjectile::CProjectile
	(
		CGameWorld *pGameWorld,
		int Type,
		int Owner,
		vec2 Pos,
		vec2 Dir,
		int Span,
        int Damage,
		bool Freeze,
		bool Explosive,
		float Force,
		int SoundImpact,
		int Weapon,
        bool FastReload,
        int Layer,
        int Number
	)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Type = Type;
    m_Owner = Owner;
    m_Pos = Pos;
    m_Direction = Dir;
    m_LifeSpan = Span;
    m_Freeze = Freeze;
    m_Explosive = Explosive;
    m_Force = Force;
    m_Damage = Damage;
    m_SoundImpact = SoundImpact;
    m_Weapon = Weapon;
    m_fast_reload = FastReload;
    m_Layer = Layer;
    m_Number = Number;

    m_StartTick = Server()->Tick();
	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	if(m_LifeSpan > -2)
		GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;

		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	vec2 ColPos;
	vec2 NewPos;
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos, false);
	CCharacter *pOwnerChar = 0;


    // CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd), 1);
    if(m_Owner >= 0)
        pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

    CCharacter *pTargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, ColPos, m_Freeze ? 1.0f : 6.0f, ColPos, pOwnerChar);

    if(m_LifeSpan > -1)
        m_LifeSpan--;

    int64_t TeamMask = -1LL;




    bool isWeaponCollide = false;
	if
	(
			pOwnerChar &&
			pTargetChr &&
			pOwnerChar->IsAlive() &&
			pTargetChr->IsAlive() &&
			!pTargetChr->CanCollide(m_Owner)
			)
	{
			isWeaponCollide = true;
			//TeamMask = OwnerChar->Teams()->TeamMask( OwnerChar->Team());
	}
	if (pOwnerChar && pOwnerChar->IsAlive())
	{
			TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);
	}
    if(pTargetChr) {
        pTargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon);
    }
	if( ((pTargetChr && (pOwnerChar ? !(pOwnerChar->m_Hit&CCharacter::DISABLE_HIT_GRENADE) : g_Config.m_SvHit || m_Owner == -1 || pTargetChr == pOwnerChar)) || Collide || GameLayerClipped(CurPos)) && !isWeaponCollide)
	{
		if(m_Explosive/*??*/ && (!pTargetChr || (pTargetChr && !m_Freeze)))
		{
			GameServer()->CreateExplosion(ColPos, m_Owner, m_Weapon, true, (!pTargetChr ? -1 : pTargetChr->Team()),
			(m_Owner != -1)? TeamMask : -1LL);

            // m_fast_reload
            // g_Config.m_SvSilentXXL
            // g_Config.m_SvSilentXXL && m_fast_reload
            // 1 1
            if(g_Config.m_SvSilentXXL && m_fast_reload)
                int asdasd = 0;
            else
                GameServer()->CreateSound(ColPos, m_SoundImpact, (m_Owner != -1)? TeamMask : -1LL);
		}
		else if(pTargetChr && m_Freeze && ((m_Layer == LAYER_SWITCH && GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[pTargetChr->Team()]) || m_Layer != LAYER_SWITCH))
			pTargetChr->Freeze();
		if(Collide && m_Bouncing != 0)
		{
            vec2 TempPos = NewPos;
            vec2 TempDir = m_Direction * 4.0f;

            GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
            m_Pos = TempPos;
            m_Direction = normalize(TempDir);
            m_StartTick = Server()->Tick();
            m_Bouncing++;
		}
		else if (m_Weapon == WEAPON_GUN)
		{

            // top left in map
            /*for (int i = 0; i < 10; i++)
            {
                CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)GameServer()->m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd), TeamMask);
                if(pEvent)
                {
                    pEvent->m_X = (int)m_Direction.x*100.0f;
                    pEvent->m_Y = (int)m_Direction.x*100.0f;
                    pEvent->m_Angle = (int)(i*256.0f);
                }
            }*/


            // GameServer()->CreatePlayerSpawn(CurPos, (m_Owner != -1)? TeamMask : -1LL);
            // GameServer()->CreateDeath(CurPos, m_Owner, TeamMask);
            // GameServer()->CreateHammerHit(CurPos, TeamMask);
			GameServer()->CreateDamageInd(CurPos, -atan2(m_Direction.x, m_Direction.y), 10, (m_Owner != -1)? TeamMask : -1LL);
			GameServer()->m_World.DestroyEntity(this);

		}
		else
			if (!m_Freeze)
				GameServer()->m_World.DestroyEntity(this);
	}
	if(m_LifeSpan == -1)
	{
		GameServer()->m_World.DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CCharacter* pSnapChar = GameServer()->GetPlayerChar(SnappingClient);
	int Tick = (Server()->Tick()%Server()->TickSpeed())%((m_Explosive)?6:20);
	if (pSnapChar && pSnapChar->IsAlive() && (m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[pSnapChar->Team()] && (!Tick)))
		return;

	if(pSnapChar && m_Owner != -1 && !pSnapChar->CanCollide(m_Owner))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);


}

// DDRace

void CProjectile::SetBouncing(int Value)
{
	m_Bouncing = Value;
}
