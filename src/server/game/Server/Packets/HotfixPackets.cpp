/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
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

#include "HotfixPackets.h"
#include "PacketUtilities.h"

void WorldPackets::Hotfix::DBQueryBulk::Read()
{
    _worldPacket >> TableHash;

    uint32 count = _worldPacket.ReadBits(13);

    Queries.resize(count);
    for (uint32 i = 0; i < count; ++i)
    {
        _worldPacket >> Queries[i].GUID;
        _worldPacket >> Queries[i].RecordID;
    }
}

WorldPacket const* WorldPackets::Hotfix::DBReply::Write()
{
    _worldPacket << TableHash;
    _worldPacket << RecordID;
    _worldPacket << Timestamp;

    _worldPacket.WriteBit(Allow);
    _worldPacket << static_cast<uint32>(Data.size());
    _worldPacket.append(Data);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Hotfix::AvailableHotfixes::Write()
{
    _worldPacket << int32(HotfixCacheVersion);
    _worldPacket << static_cast<uint32>(Hotfixes.size());
    for (auto const& hotfixEntry : Hotfixes)
        _worldPacket << uint64(hotfixEntry.first);

    return &_worldPacket;
}

void WorldPackets::Hotfix::HotfixRequest::Read()
{
    auto hotfixCount = _worldPacket.read<uint32>();
    if (hotfixCount > sDB2Manager.GetHotfixData().size())
        throw PacketArrayMaxCapacityException(hotfixCount, sDB2Manager.GetHotfixData().size());

    Hotfixes.resize(hotfixCount);
    for (auto& hotfixId : Hotfixes)
        _worldPacket >> hotfixId;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Hotfix::HotfixData const& hotfixData)
{
    data << uint64(hotfixData.ID);
    data << int32(hotfixData.RecordID);
    data.WriteBit(hotfixData.Data.has_value());
    if (hotfixData.Data.has_value())
    {
        data << uint32(hotfixData.Data->size());
        data.append(*hotfixData.Data);
    }
    else
        data << uint32(0);

    return data;
}

WorldPacket const* WorldPackets::Hotfix::HotfixResponse::Write()
{
    _worldPacket << uint32(Hotfixes.size());
    for (auto const& hotfix : Hotfixes)
        _worldPacket << hotfix;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Hotfix::HotfixMessage::Write()
{
    _worldPacket << uint32(Hotfixes.size());
    for (auto const& hotfix : Hotfixes)
        _worldPacket << hotfix;

    return &_worldPacket;
}
