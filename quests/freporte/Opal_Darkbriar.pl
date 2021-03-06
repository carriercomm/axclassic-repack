############################################
# ZONE: East Freeport (freporte)
# DATABASE: PEQ-Velious
# LAST EDIT DATE: May 26,2005
# VERSION: 1.0
# BASE QUEST BY: PEQ Quest Team
# DEVELOPER: MWMDRAGON
#
# *** NPC INFORMATION ***
#
# NAME: Opal_Darkbriar
# ID: 9073
# TYPE: Guild Master Wizard
# RACE: High Elf
# LEVEL: 61
#
# *** ITEMS GIVEN OR TAKEN ***
#
# A tattered note ID-18739
# Patched Violet Robe ID-13558
# Testament of Vanear ID-13991
# Marnek's Wand ID- MISSING
# Sapphire of Souls ID- MISSING
# Wand of the Burning Dead ID- MISSING
#
# *** QUESTS INVOLVED IN ***
#
#1 Wizard Newbie Note
#2 Wand of Burning Dead (BROKEN - Items Needed)
#
# *** QUESTS AVAILABLE TO ***
#
#1 Wizard
#2 Necromancer , Wizard
#
############################################


sub EVENT_SAY
{
  if ($text=~/Hail/i)
  {
  quest::say("Hello.");
  }

}


sub EVENT_ITEM
{
  # A tattered note ID-18739
  if ($item1=="18742")
  {
  quest::say("welcome young necromancer to the order of Dismal Rage. Be careful where you go and who you talk to as our presents in these parts are frowned upon by most. Take this robe and wear it proudly $name.");
  # Patched Violet Robe ID-13558
  quest::summonitem("13566");
  quest::ding();
  quest::faction(21, 10);
  quest::faction(71, 10);
  quest::faction(322,10);
  quest::faction(86, 10);
  quest::faction(235, 10);
  quest::exp("1000");
  quest::rebind(10,-188,-69,-94);
  }
    else {
   quest::say("I have no use for this.");
   plugin::return_items(\%itemcount);
   }
}
#END of FILE Zone:freporte  ID:9073 -- Opal_Darkbriar

