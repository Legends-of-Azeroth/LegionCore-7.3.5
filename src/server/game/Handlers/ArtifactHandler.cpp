/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "ArtifactPackets.h"
#include "GameTables.h"

void WorldSession::HandleArtifactAddPower(WorldPackets::Artifact::AddPower& artifactAddPower)
{
    if (!_player->GetGameObjectIfCanInteractWith(artifactAddPower.GameObjectGUID, GAMEOBJECT_TYPE_ARTIFACT_FORGE))
        return;

    Item* artifact = _player->GetItemByGuid(artifactAddPower.ArtifactGUID);
    if (!artifact)
        return;

    auto categoryID = ARTIFACT_CATEGORY_CLASS;
    uint32 artifactID = artifact->GetTemplate()->GetArtifactID();
        if (ArtifactEntry const* entry = sArtifactStore.LookupEntry(artifactID))
            categoryID = static_cast<ArtifactCategory>(entry->ArtifactCategoryID);

    uint64 xpCost = 0;
    if (GtArtifactLevelXPEntry const* cost = sArtifactLevelXPGameTable.GetRow(artifact->GetTotalPurchasedArtifactPowers() + 1))
        xpCost = uint64(artifact->GetModifier(ITEM_MODIFIER_ARTIFACT_TIER) == 1 ? cost->XP2 : cost->XP);

    if (xpCost > artifact->GetUInt64Value(ITEM_FIELD_ARTIFACT_XP))
        return;

    if (artifactAddPower.Powers.empty())
        return;

    if (categoryID == ARTIFACT_CATEGORY_FISH)
        artifact->ActivateFishArtifact(artifactID);

    ItemDynamicFieldArtifactPowers const* artifactPower = artifact->GetArtifactPower(artifactAddPower.Powers[0].PowerID);
    if (!artifactPower)
        return;

    ArtifactPowerEntry const* artifactPowerEntry = sArtifactPowerStore.LookupEntry(artifactPower->ArtifactPowerId);
    if (!artifactPowerEntry)
        return;

    uint32 MaxRank = artifactPowerEntry->MaxPurchasableRank;
    if (artifact->GetModifier(ITEM_MODIFIER_ARTIFACT_TIER) && (artifactPowerEntry->Flags & ARTIFACT_POWER_FLAG_HAS_RANK) && !artifactPowerEntry->Tier)
        MaxRank += 1;

    if (artifactAddPower.Powers[0].Rank != artifactPower->PurchasedRank + 1 || artifactAddPower.Powers[0].Rank > MaxRank)
        return;

    if (std::unordered_set<uint32> const* artifactPowerLinks = sDB2Manager.GetArtifactPowerLinks(artifactPower->ArtifactPowerId))
    {
        bool hasAnyLink = false;
        if ((artifactPowerEntry->Flags & ARTIFACT_POWER_FLAG_NO_LINK_REQUIRED) && artifactPowerEntry->Tier)
            hasAnyLink = true;

        for (uint32 artifactPowerLinkId : *artifactPowerLinks)
        {
            ArtifactPowerEntry const* artifactPowerLink = sArtifactPowerStore.LookupEntry(artifactPowerLinkId);
            if (!artifactPowerLink)
                continue;

            ItemDynamicFieldArtifactPowers const* artifactPowerLinkLearned = artifact->GetArtifactPower(artifactPowerLinkId);
            if (!artifactPowerLinkLearned)
                continue;

            if (artifactPowerLinkLearned->PurchasedRank >= artifactPowerLink->MaxPurchasableRank)
            {
                hasAnyLink = true;
                break;
            }
        }

        if (!hasAnyLink && !artifact->GetModifier(ITEM_MODIFIER_ARTIFACT_TIER))
            return;

        if (!hasAnyLink && artifactPowerEntry->Tier)
            return;
    }

    ArtifactPowerRankEntry const* artifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower->ArtifactPowerId, artifactPower->CurrentRankWithBonus + 1 - 1); // need data for next rank, but -1 because of how db2 data is structured
    if (!artifactPowerRank)
        return;

    ItemDynamicFieldArtifactPowers newPower = *artifactPower;
    ++newPower.PurchasedRank;
    ++newPower.CurrentRankWithBonus;
    artifact->SetArtifactPower(&newPower);

    if (artifact->IsEquipped())
    {
        _player->ApplyArtifactPowerRank(artifact, artifactPowerRank, true);

        for (ItemDynamicFieldArtifactPowers const& power : artifact->GetArtifactPowers())
        {
            ArtifactPowerEntry const* scaledArtifactPowerEntry = sArtifactPowerStore.LookupEntry(power.ArtifactPowerId);
            if (!scaledArtifactPowerEntry)
                continue;

            if (!(scaledArtifactPowerEntry->Flags & ARTIFACT_POWER_FLAG_SCALES_WITH_NUM_POWERS))
                continue;

            ArtifactPowerRankEntry const* scaledArtifactPowerRank = sDB2Manager.GetArtifactPowerRank(scaledArtifactPowerEntry->ID, 0);
            if (!scaledArtifactPowerRank)
                continue;

            ItemDynamicFieldArtifactPowers newScaledPower = power;
            ++newScaledPower.CurrentRankWithBonus;
            artifact->SetArtifactPower(&newScaledPower);

            _player->ApplyArtifactPowerRank(artifact, scaledArtifactPowerRank, false);
            _player->ApplyArtifactPowerRank(artifact, scaledArtifactPowerRank, true);
        }
    }

    artifact->SetUInt64Value(ITEM_FIELD_ARTIFACT_XP, artifact->GetUInt64Value(ITEM_FIELD_ARTIFACT_XP) - xpCost);
    artifact->SetState(ITEM_CHANGED, _player);
    _player->UpdateAchievementCriteria(CRITERIA_TYPE_OPEN_ARTIFACT_POWERS, 1);
}


void WorldSession::HandleArtifactAddRelicTalent(WorldPackets::Artifact::ArtifactAddRelicTalent& packet)
{
    if (!_player->GetGameObjectIfCanInteractWith(packet.GameObjectGUID, GAMEOBJECT_TYPE_ARTIFACT_FORGE))
        return;

    Item* artifact = _player->GetItemByGuid(packet.ArtifactGUID);
    if (!artifact)
        return;

    auto relicks = artifact->GetArtifactSockets();
    if (relicks.find(packet.SlotIndex) == relicks.end()) // can it?
        return;

    if (auto gem = artifact->GetGem(packet.SlotIndex - 2))
    {
        if (!gem || !gem->ItemId)
            return;
    }

    if ((1 << packet.TalentIndex) & relicks[packet.SlotIndex].firstTier)
        return;

    uint32 reqLevel = 0;
    if (packet.TalentIndex > 0 )
        reqLevel = 60 + (packet.SlotIndex - 2) * 3;
    if (packet.TalentIndex > 2)
        reqLevel += 9;

    if (reqLevel > artifact->GetTotalPurchasedArtifactPowers())
    {
        _player->SendEquipError(EQUIP_ERR_SCALING_STAT_ITEM_LEVEL_TOO_LOW);
        return;
    }

    artifact->SetState(ITEM_CHANGED, _player);

    uint8 offset = (packet.SlotIndex - 2) * 6;
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.SlotIndex].unk1);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, packet.SlotIndex);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.SlotIndex].firstTier | (1<<packet.TalentIndex));
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.SlotIndex].secondTier);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.SlotIndex].thirdTier);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.SlotIndex].additionalThirdTier);

    _player->UpdateAchievementCriteria(CRITERIA_TYPE_RELIC_TALENT_UNLOCKED, 1739, 1);

    artifact->AddOrRemoveSocketTalent(packet.TalentIndex, true, packet.SlotIndex);
}

void WorldSession::HandleArtifactAttuneSocketedRelic(WorldPackets::Artifact::ArtifactAttuneSocketedRelic& packet)
{ 
    if (!_player->GetGameObjectIfCanInteractWith(packet.GameObjectGUID, GAMEOBJECT_TYPE_ARTIFACT_FORGE))
        return;

    Item* artifact = _player->GetItemByGuid(packet.ArtifactGUID);
    if (!artifact)
        return;

    if (auto gem = artifact->GetGem(packet.RelicSlotIndex - 2))
    {
        if (!gem || !gem->ItemId)
            return;
    }
    artifact->SetState(ITEM_CHANGED, _player);

    auto relicks = artifact->GetArtifactSockets();
    if (relicks.find(packet.RelicSlotIndex) == relicks.end())
    {
        artifact->CreateSocketTalents(packet.RelicSlotIndex);
        return;
    }

    uint8 offset = (packet.RelicSlotIndex - 2) * 6;
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.RelicSlotIndex].unk1);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, packet.RelicSlotIndex);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.RelicSlotIndex].firstTier);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.RelicSlotIndex].secondTier);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.RelicSlotIndex].thirdTier);
    artifact->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[packet.RelicSlotIndex].additionalThirdTier);
}

void WorldSession::HandleArtifactAttunePreviewRelic(WorldPackets::Artifact::ArtifactAttunePreviewdRelic& packet)
{
    if (!_player->GetGameObjectIfCanInteractWith(packet.GameObjectGUID, GAMEOBJECT_TYPE_ARTIFACT_FORGE))
        return;

    Item* socket = _player->GetItemByGuid(packet.RelicGUID);
    if (!socket)
        return;

    socket->SetBinding(true);
    socket->SetState(ITEM_CHANGED, _player);

    auto relicks = socket->GetArtifactSockets();
    if (relicks.find(2) == relicks.end())
    {
        socket->CreateSocketTalents(2);
        return;
    }

    uint8 offset = 0;
    socket->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[2].unk1);
    socket->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, 2);
    socket->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[2].firstTier);
    socket->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[2].secondTier);
    socket->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[2].thirdTier);
    socket->SetDynamicValue(ITEM_DYNAMIC_FIELD_RELIC_TALENT_DATA, offset++, relicks[2].additionalThirdTier);
}

void WorldSession::HandleArtifactConfirmRespec(WorldPackets::Artifact::ConfirmRespec& confirmArtifactRespec)
{
    if (!_player->GetNPCIfCanInteractWith(confirmArtifactRespec.NpcGUID, UNIT_NPC_FLAG_ARTIFACT_POWER_RESPEC))
        return;

    Item* artifact = _player->GetItemByGuid(confirmArtifactRespec.ArtifactGUID);
    if (!artifact)
        return;

    uint64 xpCost = 0;
    if (GtArtifactLevelXPEntry const* cost = sArtifactLevelXPGameTable.GetRow(artifact->GetTotalPurchasedArtifactPowers() + 1))
        xpCost = uint64(artifact->GetModifier(ITEM_MODIFIER_ARTIFACT_TIER) == 1 ? cost->XP2 : cost->XP);

    if (xpCost > artifact->GetUInt64Value(ITEM_FIELD_ARTIFACT_XP))
        return;

    uint64 newAmount = artifact->GetUInt64Value(ITEM_FIELD_ARTIFACT_XP) - xpCost;
    for (uint32 i = 0; i <= artifact->GetTotalPurchasedArtifactPowers(); ++i)
        if (GtArtifactLevelXPEntry const* cost = sArtifactLevelXPGameTable.GetRow(i))
            newAmount += uint64(artifact->GetModifier(ITEM_MODIFIER_ARTIFACT_TIER) == 1 ? cost->XP2 : cost->XP);

    for (ItemDynamicFieldArtifactPowers const& artifactPower : artifact->GetArtifactPowers())
    {
        uint8 oldPurchasedRank = artifactPower.PurchasedRank;
        if (!oldPurchasedRank)
            continue;

        ItemDynamicFieldArtifactPowers newPower = artifactPower;
        newPower.PurchasedRank -= oldPurchasedRank;
        newPower.CurrentRankWithBonus -= oldPurchasedRank;
        artifact->SetArtifactPower(&newPower);

        if (artifact->IsEquipped())
            if (ArtifactPowerRankEntry const* artifactPowerRank = sDB2Manager.GetArtifactPowerRank(artifactPower.ArtifactPowerId, 0))
                _player->ApplyArtifactPowerRank(artifact, artifactPowerRank, false);
    }

    for (ItemDynamicFieldArtifactPowers const& power : artifact->GetArtifactPowers())
    {
        ArtifactPowerEntry const* scaledArtifactPowerEntry = sArtifactPowerStore.AssertEntry(power.ArtifactPowerId);
        if (!(scaledArtifactPowerEntry->Flags & ARTIFACT_POWER_FLAG_SCALES_WITH_NUM_POWERS))
            continue;

        ArtifactPowerRankEntry const* scaledArtifactPowerRank = sDB2Manager.GetArtifactPowerRank(scaledArtifactPowerEntry->ID, 0);
        if (!scaledArtifactPowerRank)
            continue;

        ItemDynamicFieldArtifactPowers newScaledPower = power;
        newScaledPower.CurrentRankWithBonus = 0;
        artifact->SetArtifactPower(&newScaledPower);

        _player->ApplyArtifactPowerRank(artifact, scaledArtifactPowerRank, false);
    }

    artifact->SetUInt64Value(ITEM_FIELD_ARTIFACT_XP, newAmount);
    artifact->SetState(ITEM_CHANGED, _player);
}

void WorldSession::HandleArtifactSetAppearance(WorldPackets::Artifact::SetAppearance& artifactSetAppearance)
{
    if (!_player->GetGameObjectIfCanInteractWith(artifactSetAppearance.GameObjectGUID, GAMEOBJECT_TYPE_ARTIFACT_FORGE))
        return;

    ArtifactAppearanceEntry const* artifactAppearance = sArtifactAppearanceStore.LookupEntry(artifactSetAppearance.AppearanceID);
    if (!artifactAppearance)
        return;

    Item* artifact = _player->GetItemByGuid(artifactSetAppearance.ArtifactGUID);
    if (!artifact)
        return;

    ArtifactAppearanceSetEntry const* artifactAppearanceSet = sArtifactAppearanceSetStore.LookupEntry(artifactAppearance->ArtifactAppearanceSetID);
    if (!artifactAppearanceSet || artifactAppearanceSet->ArtifactID != artifact->GetTemplate()->GetArtifactID())
        return;

    if (!sConditionMgr->IsPlayerMeetingCondition(_player, artifactAppearance->UnlockPlayerConditionID))
        return;

    artifact->SetAppearanceModId(artifactAppearance->ItemAppearanceModifierID);
    artifact->SetModifier(ITEM_MODIFIER_ARTIFACT_APPEARANCE_ID, artifactAppearance->ID);
    artifact->SetState(ITEM_CHANGED, _player);
    Item* childItem = _player->GetChildItemByGuid(artifact->GetChildItem());
    // Disable, buged update packet
    if (childItem)
    {
        childItem->SetAppearanceModId(artifactAppearance->ItemAppearanceModifierID);
        childItem->SetState(ITEM_CHANGED, _player);
    }

    if (artifact->IsEquipped())
    {
        _player->SetVisibleItemSlot(artifact->GetSlot(), artifact);
        if (childItem)
            _player->SetVisibleItemSlot(childItem->GetSlot(), childItem);

        if (artifactAppearance->OverrideShapeshiftDisplayID && artifactAppearance->OverrideShapeshiftFormID && _player->GetShapeshiftForm() == ShapeshiftForm(artifactAppearance->OverrideShapeshiftFormID))
            _player->RestoreDisplayId();
    }
}
