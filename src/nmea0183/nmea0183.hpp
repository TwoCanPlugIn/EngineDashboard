//
// This file is part of Engine Dashboard, a plugin for OpenCPN.
// based on the original version of the dashboard.
// Author: Steven Adler
//
/***************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  NMEA0183 Support Classes
 * Author:   Samuel R. Blackburn, David S. Register
 *
 ***************************************************************************
 *   Copyright (C) 2010 by Samuel R. Blackburn, David S Register           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 *
 *   S Blackburn's original source license:                                *
 *         "You can use it any way you like."                              *
 *   More recent (2010) license statement:                                 *
 *         "It is BSD license, do with it what you will"                   *
 */


#if ! defined( NMEA_0183_CLASS_HEADER)
#define NMEA_0183_CLASS_HEADER

/*
** Author: Samuel R. Blackburn
** CI$: 76300,326
** Internet: sammy@sed.csc.com
**
** You can use it any way you like.
*/

/*
** General Purpose Classes
*/

#include "sentence.hpp"
#include "response.hpp"
#include "rpm.hpp"
#include "rsa.hpp"
#include "xdr.hpp"

WX_DECLARE_LIST(RESPONSE, MRL);

class NMEA0183
{

   private:

      SENTENCE sentence;

      void initialize( void);

   protected:

      MRL response_table;

      void set_container_pointers( void);
      void sort_response_table( void);

   public:

      NMEA0183();
      virtual ~NMEA0183();

      /*
      ** NMEA 0183 used by the engine dashboard.
	  ** Almost all of the sentences used by thge original dashboard have been omitted
      */
	  XDR Xdr;
	  RPM Rpm;
	  RSA Rsa;

      wxString ErrorMessage; // Filled when Parse returns FALSE
      wxString LastSentenceIDParsed; // ID of the lst sentence successfully parsed
      wxString LastSentenceIDReceived; // ID of the last sentence received, may not have parsed successfully

      wxString TalkerID;
      wxString ExpandedTalkerID;

//      MANUFACTURER_LIST Manufacturers;

      bool IsGood( void) const;
      bool Parse( void);
      bool PreParse( void);

      NMEA0183& operator << (wxString& source);
      NMEA0183& operator >> (wxString& destination);
};

#endif // NMEA_0183_CLASS_HEADER
