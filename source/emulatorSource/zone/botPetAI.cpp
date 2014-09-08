/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2004  EQEMu Development Team (http://eqemu.org)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef EQBOTS

#include "../common/debug.h"
#include <iostream>
using namespace std;
#include <iomanip>
using namespace std;
#include <stdlib.h>
#include <math.h>
#include "npc.h"
#include "masterentity.h"
#include "NpcAI.h"
#include "map.h"
#include "../common/moremath.h"
#include "parser.h"
#include "StringIDs.h"
#include "../common/MiscFunctions.h"
#include "../common/rulesys.h"

#if !defined(NEW_LoadSPDat) && !defined(DB_LoadSPDat)
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif

extern EntityList entity_list;

extern Zone *zone;
extern Parser * parse;

const int Z_AGGRO=10;

const int MobAISpellRange=100; // max range of buffs
const int SpellType_Nuke=1;
const int SpellType_Heal=2;
const int SpellType_Root=4;
const int SpellType_Buff=8;
const int SpellType_Escape=16;
const int SpellType_Pet=32;
const int SpellType_Lifetap=64;
const int SpellType_Snare=128;
const int SpellType_DOT=256;
const int SpellType_Dispel=512;

const int SpellTypes_Detrimental = SpellType_Nuke|SpellType_Root|SpellType_Lifetap|SpellType_Snare|SpellType_DOT|SpellType_Dispel;
const int SpellTypes_Beneficial = SpellType_Heal|SpellType_Buff|SpellType_Escape|SpellType_Pet;
//int randInt = MakeRandomInt(1, 100);
#define SpellType_Any		0xFFFF
#ifdef _EQDEBUG
	#define MobAI_DEBUG_Spells	-1
#else
	#define MobAI_DEBUG_Spells	-1
#endif


// Franck-add: EQoffline
// AI, for the Bot pets
void Mob::PET_Process() {
	_ZP(Mob_PET_Process);
	int randInt = MakeRandomInt(1, 100);
	float runspeed = this->GetRunspeed();

	if (!petCharmed){
	// of course, if we're not a pet
	    if((!IsPet()) || (!GetID()) || (!IsFamiliar()))
		return;

	    if((!GetOwner()) || (!GetOwnerID()) || (!BotOwner)){
		Kill();
		return;
	    } 
	}
	
	if(!IsAIControlled())
		return;
 
	if(!(AIthink_timer->Check() || attack_timer.Check(false)))
		return;
 
	if(IsCasting())
		return;
 
	// if our owner isn't a pet or if he is not a client...
	if(!GetOwner()->IsBot())
		return;

///Engaged
	if(GetOwner()->IsEngaged() && IsEngaged())
	{
		_ZP(Bot_PET_Process_IsEngaged);
        if (IsRooted())
            CastToNPC()->SetTarget(hate_list.GetClosest(this));
        else
            CastToNPC()->SetTarget(hate_list.GetTop(this));

        if(!target)
            return;

	//FaceTarget(target);
	
	//Angelox: Stop attacking if the target has less than 10 percent hp, this is to avoid null targets after kill.
	if(target->GetHPRatio() < 10)
			return;
			
	// Stop attacking if the target is enraged
	if(!BehindMob(target, GetX(), GetY()) && target->IsEnraged())
			return;
 
	// Let's check if we have a los with our target.
	// If we don't, our hate_list is wiped.
	// It causes some cpu stress but without it, it was causing the bot/pet to aggro behind wall, floor etc... 
        if(/* !CheckLosFN(target) ||*/ target->IsMezzed() || !IsAttackAllowed(target)) {
			WipeHateList();
            CastToNPC()->SetTarget(GetOwner());
            return;
        }

		// Lets see if we can let the main tank build a little aggro
		if(GetBotRaidID())
		{
			BotRaids *br = entity_list.GetBotRaidByMob(GetOwner());
			if(br)
			{
				if(br->GetBotMainTank() && (br->GetBotMainTank() != this))
				{
					if(br->GetBotMainTarget() && (br->GetBotMainTarget()->GetHateAmount(br->GetBotMainTank()) < 5000))
					{
						if(target == br->GetBotMainTarget())
						{
							return;
						}
					}
				}
			}
		}
	      if (target){
		bool is_combat_range = CombatRange(target);
 
		// Ok, we're engaged, each class type has a special AI
		// Only melee class will go to melee. Casters and healers will stay behind, following the leader by default.
		// I should probably make the casters staying in place so they can cast..
 
		// Ok, we 're a melee or any other class lvl<12. Yes, because after it becomes hard to go in melee for casters.. even for bots..
		if( is_combat_range )
		{
			AImovement_timer->Check();
			if(IsMoving())
			{
				SetRunAnimSpeed(0);
				SetHeading(target->GetHeading());
				if(moved) {
					moved=false;
					SetMoving(false);
					if (!IsMoving()){
					  //BotOwner->CastToClient()->Message(15, "FaceTarget-A.");
					  BotFaceTarget();
					  SendPosition();
					}
				}
			}
			// we can't fight if we don't have a target, are stun/mezzed or dead..
			if(target && !IsStunned() && !IsMezzed() && (GetAppearance() != eaDead)) 
			{
				if(attack_timer.Check())  // check the delay on the attack
				{
					// Special attack
					CastToNPC()->DoClassAttacks(target); 

					if(Attack(target, 13))			// try the main hand
					if (target)					// Do we still have a target?
					{
						// We're a pet so we re able to dual attack
						sint32 RandRoll = MakeRandomInt(0, 99);	
						if (CanThisClassDoubleAttack() && (RandRoll < (GetLevel() + NPCDualAttackModifier)))	
						{
							if(Attack(target, 13)) 
							{}
						}
					}
 
					// Ok now, let's check pet's offhand. CastToClient()
					if (attack_dw_timer.Check() && GetOwnerID() && GetOwner() && ((GetOwner()->GetClass() == MAGICIAN) || (GetOwner()->GetClass() == NECROMANCER) || (GetOwner()->GetClass() == SHADOWKNIGHT) || (GetOwner()->GetClass() == BEASTLORD))) 
					{
						if(GetOwner()->GetLevel() >= 24)
						{
							float DualWieldProbability = (GetSkill(DUAL_WIELD) + GetLevel()) / 400.0f;
                            DualWieldProbability -= MakeRandomFloat(0, 1);
							if(DualWieldProbability < 0){
                                Attack(target, 14);
                                if (CanThisClassDoubleAttack())
                                {
                                    sint32 RandRoll = rand()%100;
                                    if (RandRoll < (GetLevel() + 20))
                                    {
                                        Attack(target, 14);
                                    }
                                }
							}
						}
					}
					if(!GetOwner())
						return;
				}
                // See if the pet can cast any spell
                Bot_AI_EngagedCastCheck();
			}	
		}// end of the combat in range
		
		    else
		    {
			// Now, if we cannot reach our target
			if (!HateSummon()) 
			{
				if(target && Bot_AI_PursueCastCheck()) 
				{}
				else if (target && AImovement_timer->Check()) 
				{
				    if (!IsRooted()){
					  float dist2 = DistNoRoot(*target);
					    if(!CheckLosFN(target)){
						if(!check_attack_timer.Enabled())
						    check_attack_timer.Start(5000, false); //Try to reach target for five seconds, then if not there, warp.
			  
						if (zone->pathing){
						    float walkspeed = .25;
						    animation = 21 * runspeed;
						    SetRunAnimSpeed(this->animation);
						    bool WaypointChanged, NodeReached;
						    VERTEX Goal = UpdatePath(GetTarget()->GetX(), GetTarget()->GetY(), GetTarget()->GetZ(),walkspeed, WaypointChanged, NodeReached);
			    
						    if(WaypointChanged)
							tar_ndx = 20;
				
						    else{
						      if(check_attack_timer.Check(false)) {
							  Warp(target->GetX(), target->GetY(), target->GetZ());
							  CalculateNewPosition2(target->GetX(), target->GetY(), target->GetZ(), runspeed, false);
						      }
						      else
							  CalculateNewPosition2(Goal.x, Goal.y, this->GetZ(), this->runspeed);
						    }
						}
						else if (!zone->pathing){
						    if(check_attack_timer.Check(false)) {
							Warp(target->GetX(), target->GetY(), target->GetZ());
							CalculateNewPosition2(target->GetX(), target->GetY(), target->GetZ(), runspeed, false);
						    }
						    else
							CalculateNewPosition2(GetTarget()->GetX(), GetTarget()->GetY(), GetTarget()->GetZ(), runspeed);
						}
						else if (check_attack_timer.Enabled())
						  check_attack_timer.Disable();
					    }
					    else if (dist2 > 40){
					      CalculateNewPosition2(GetTarget()->GetX(), GetTarget()->GetY(), GetTarget()->GetZ(), runspeed);
					    }
					    if ((IsMoving()) && (randInt <20)){
					      BotFaceTarget(target);
					      //BotOwner->CastToClient()->Message(15, "PetFaceTarget-B.");
					    }
					    else if (!IsMoving()){
					      SendPosition();
					      //BotOwner->CastToClient()->Message(15, "PetSendPosition-B.");
					    }
				    }
			      }
			}
		    }
	      }
		//break owners invis when pets attack
		if(GetOwner()->invisible) {
			mlog(COMBAT__ATTACKS, "Removing invisibility due to melee attack.");
			GetOwner()->BuffFadeByEffect(SE_Invisibility);
			GetOwner()->BuffFadeByEffect(SE_Invisibility2);
			GetOwner()->invisible = false;
		}
		if(GetOwner()->invisible_undead) {
			mlog(COMBAT__ATTACKS, "Removing invisibility vs. undead due to melee attack.");
			GetOwner()->BuffFadeByEffect(SE_InvisVsUndead);
			GetOwner()->BuffFadeByEffect(SE_InvisVsUndead2);
			GetOwner()->invisible_undead = false;
		}
		if(GetOwner()->invisible_animals){
			mlog(COMBAT__ATTACKS, "Removing invisibility vs. animals due to melee attack.");
			GetOwner()->BuffFadeByEffect(SE_InvisVsAnimals);
			GetOwner()->invisible_animals = false;
		}
	}
///End Engaged

	// Franck: EQoffline
	// Ok if we're not engaged, what's happening..
	else{
		if(target != GetOwner()) {
			CastToNPC()->SetTarget(GetOwner());
			  if(target != GetOwner()) {
			    Kill();
			    return;
			  }
		}
		if(!IsMoving()) {
			Bot_AI_IdleCastCheck();
			  if(target != GetOwner()) {
			    Kill();
			    return;
			  }
			animation = 0;
		}
		
		if (check_attack_timer.Enabled()){
		 //BotSay("Disable Timer2.");
		 check_attack_timer.Disable();
		 //FaceTarget();
		}

		if(AImovement_timer->Check()) {
			switch(pStandingPetOrder) {
				case SPO_Follow:
					{
						float dist = DistNoRoot(*target);
						SetRunAnimSpeed(0);
						if (dist > 15000){ //Angelox: We need to avoid lag so, if the pet falls too far behind, warp'm
						  BotWarp(target->GetX(), target->GetY(), target->GetZ());
						  //BotOwner->CastToClient()->Message(15, "Pet Follow Warp.");
						}
						else if(dist > 80) {
						    if ((zone->pathing) && (!CheckLosFN(target))){
							float speed = GetBaseRunspeed();
							//animation = 21 * speed;
							bool WaypointChanged, NodeReached;
							VERTEX Goal = UpdatePath(GetTarget()->GetX(), GetTarget()->GetY(), GetTarget()->GetZ(),speed, WaypointChanged, NodeReached);
							  if(WaypointChanged)
							    tar_ndx = 20;

							CalculateNewPosition2(Goal.x, Goal.y, Goal.z, speed);
							/*if ((randInt <25) && (IsMoving())){
							  //BotOwner->CastToClient()->Message(15, "FaceTarget-C.");
							  FaceTarget();
							}*/
						    }
						    else if (CheckLosFN(target)){
							CalculateNewPosition2(target->GetX(), target->GetY(), target->GetZ(), runspeed, false);
							/*if ((randInt <25) && (IsMoving())){
							  //BotOwner->CastToClient()->Message(15, "FaceTarget-D.");
							  FaceTarget();
							}*/
						    }
						    else{
							//Warp(target->GetX(), target->GetY(), target->GetZ());
							CalculateNewPosition2(target->GetX(), target->GetY(), target->GetZ(), runspeed, false);
 						    }
						        
						}
						else if(IsMoving())  {
						    //SetHeading(target->GetHeading());
							if(moved) {
								moved=false;
								SetMoving(false);
								if (!IsMoving()){
								  BotFaceTarget();
								  animation = 0;
								  //BotOwner->CastToClient()->Message(15, "PetSendPosition-A.");
								  SendPosition();
								}
							}
						}
					}
					break;
				case SPO_Sit:
					SetAppearance(eaSitting);
					break;
				case SPO_Guard:
					CastToNPC()->NextGuardPosition();
					break;
			}
		}
	}
}

#endif //EQBOTS