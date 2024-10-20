/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef TRINITY_HOMEMOVEMENTGENERATOR_H
#define TRINITY_HOMEMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "PathGenerator.h"

template <class T>
class HomeMovementGenerator : public MovementGeneratorMedium< T, HomeMovementGenerator<T> >
{
    public:

        explicit HomeMovementGenerator() : _path(nullptr), _arrived(false), _skipToHome(false) { }
        ~HomeMovementGenerator();

        MovementGeneratorType GetMovementGeneratorType() override { return HOME_MOTION_TYPE; }

        void DoInitialize(T &);
        void DoFinalize(T &);
        void DoReset(T &);
        bool DoUpdate(T &, const uint32);

    private:
        void SetTargetLocation(T &);

        PathGenerator* _path;
        bool _arrived;
        bool _skipToHome;
};
#endif

