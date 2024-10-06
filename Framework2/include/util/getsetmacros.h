/*
Copyright 2014 Alejandro Cosin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Taken from https://gist.github.com/aminzai/2706798

#ifndef _GETSETMACROS_H_
#define _GETSETMACROS_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES

// CLASS FORWARDING

// NAMESPACE

// DEFINES
#define GET(Type, MemberName, FaceName) \
          const Type &get##FaceName() const { return MemberName; }

#define GETCOPY(Type, MemberName, FaceName) \
          const Type get##FaceName() const { return MemberName; }

#define REF(Type, MemberName, FaceName) \
          Type &ref##FaceName() { return MemberName; }

#define REFCOPY(Type, MemberName, FaceName) \
          Type ref##FaceName() { return MemberName; }

#define REF_PTR(Type, MemberName, FaceName) \
          Type *ref##FaceName() { return MemberName; }

#define GET_RETURN_PTR(Type, MemberName, FaceName) \
          const Type *get##FaceName() const { return &MemberName; }

#define REF_RETURN_PTR(Type, MemberName, FaceName) \
          Type *ref##FaceName() { return &MemberName; }

#define GET_PTR(Type, MemberName, FaceName) \
          const Type *get##FaceName() const { return MemberName; }

#define SET(Type, MemberName, FaceName) \
          void set##FaceName(const Type &value) { MemberName = value; }

#define SETMOVE(Type, MemberName, FaceName) \
          void set##FaceName(Type&& value) { MemberName = move(value); }

#define SET_PTR(Type, MemberName, FaceName) \
          void set##FaceName(Type *value) { MemberName = value; }

#define REF_SET(Type, MemberName, FaceName) \
          Type &ref##FaceName() { return MemberName; }; \
          void  set##FaceName(const Type &value) { MemberName = value; }

#define REFCOPY_SET(Type, MemberName, FaceName) \
          Type ref##FaceName() { return MemberName; }; \
          void set##FaceName(const Type &value) { MemberName = value; }

#define GET_SET(Type, MemberName, FaceName) \
          const Type &get##FaceName() const { return MemberName; }; \
          void        set##FaceName(const Type &value) { MemberName = value; }

#define GETCOPY_SET(Type, MemberName, FaceName) \
          const Type get##FaceName() const { return MemberName; }; \
          void        set##FaceName(const Type &value) { MemberName = value; }

#define GET_PTR_SET(Type, MemberName, FaceName) \
          Type *get##FaceName() const { return MemberName; }; \
          void  set##FaceName(const Type &value) { MemberName = value; }

#define GET_PTR_SET_PTR(Type, MemberName, FaceName) \
          const Type *get##FaceName() const { return MemberName; }; \
          void  set##FaceName(Type *value) { MemberName = value; }

#define REF_PTR_SET_PTR(Type, MemberName, FaceName) \
          Type *ref##FaceName() { return MemberName; }; \
          void  set##FaceName(Type *value) { MemberName = value; }

#define GET_CPTR_SET(Type, MemberName, FaceName) \
          const Type *get##FaceName() const { return MemberName; }; \
          void        set##FaceName(const Type &value) { MemberName = value; }

#define GET_ARRAY_AS_POINTER(Type, MemberName, FaceName) \
          const Type* get##FaceName() const { return &MemberName[0]; }

#define REF_ARRAY_AS_POINTER(Type, MemberName, FaceName) \
          Type* ref##FaceName() { return &MemberName[0]; }

#endif _GETSETMACROS_H_
