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


#include "nmea0183.h"

/*
** Author: Samuel R. Blackburn
** CI$: 76300,326
** Internet: sammy@sed.csc.com
**
** You can use it any way you like.
*/

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(MRL);


NMEA0183::NMEA0183()
{
   initialize();

   response_table.Append((RESPONSE *)&Rpm);
   response_table.Append((RESPONSE *)&Rsa);
   response_table.Append((RESPONSE *)&Xdr);
   sort_response_table();
   set_container_pointers();
}

NMEA0183::~NMEA0183()
{
   initialize();
}

void NMEA0183::initialize( void)
{
//   ASSERT_VALID( this);

   ErrorMessage.Empty();
}

void NMEA0183::set_container_pointers( void)
{
//   ASSERT_VALID( this);

   int index = 0;
   int number_of_entries_in_table = response_table.GetCount();

   RESPONSE *this_response = (RESPONSE *) NULL;

   index = 0;

   while( index < number_of_entries_in_table)
   {
      this_response = (RESPONSE *) response_table[ index ];

      this_response->SetContainer( this);

      index++;
   }
}

void NMEA0183::sort_response_table( void)
{
//   ASSERT_VALID( this);

/*
   int index = 0;
   int number_of_entries_in_table = response_table.GetSize();

   RESPONSE *this_response = (RESPONSE *) NULL;
   RESPONSE *that_response = (RESPONSE *) NULL;

   bool sorted = FALSE;

   while( sorted == FALSE)
   {
      sorted = TRUE;

      index = 0;

      while( index < number_of_entries_in_table)
      {
         this_response = (RESPONSE *) response_table.Item( index    );
         that_response = (RESPONSE *) response_table.Item( index + 1);

         if (this_response->Mnemonic.Compare( that_response->Mnemonic) > 0)
         {
            response_table[ index     ] = that_response;
            response_table[ index + 1 ] = this_response;

            sorted = FALSE;
         }

         index++;
      }
   }
*/
}

/*
** Public Interface
*/

bool NMEA0183::IsGood( void) const
{
//   ASSERT_VALID( this);

   /*
   ** NMEA 0183 sentences begin with $ and and with CR LF
   */

   if (sentence.Sentence[ 0 ] != '$')
   {
      return( FALSE);
   }

   /*
   ** Next to last character must be a CR
   */
   /*  This seems too harsh for cross platform work
    * 
   if (sentence.Sentence.Mid( sentence.Sentence.Len() - 2, 1) != wxString(_T("\r")))
   {
      return( FALSE);
   }

   if (sentence.Sentence.Right( 1) != _T("\n"))
   {
      return( FALSE);
   }
   */
   
   return( TRUE);
}


bool NMEA0183::PreParse( void)
{
    wxCharBuffer buf = sentence.Sentence.ToUTF8();
    if (!buf.data())                            // badly formed sentence?
        return false;
    
      if (IsGood())
      {
            wxString mnemonic = sentence.Field( 0);

      /*
            ** See if this is a proprietary field
      */

            if (mnemonic.Left( 1) == 'P')
                  mnemonic = _T("P");

            else
                  mnemonic = mnemonic.Right( 3);


            LastSentenceIDReceived = mnemonic;

            return true;
      }
      else
            return false;
}


bool NMEA0183::Parse( void)
{
   bool return_value = FALSE;

   if(PreParse())
   {

      wxString mnemonic = sentence.Field( 0);

      /*
      ** See if this is a proprietary field
      */

      if (mnemonic.Left( 1) == 'P')
      {
          mnemonic = _T("P");
      }
      else
      {
         mnemonic = mnemonic.Right( 3);
      }

      /*
      ** Set up our default error message
      */

      ErrorMessage = mnemonic;
      ErrorMessage += _T(" is an unknown type of sentence");

      LastSentenceIDReceived = mnemonic;

      RESPONSE *response_p = (RESPONSE *) NULL;


//          Traverse the response list to find a mnemonic match

       wxMRLNode *node = response_table.GetFirst();

       int comparison  = 0;

        while(node)
        {
           RESPONSE *resp = node->GetData();

            comparison = mnemonic.Cmp( resp->Mnemonic);

            if (comparison == 0)
            {
                        response_p = (RESPONSE *) resp;
                        return_value = response_p->Parse( sentence);

                        /*
                        ** Set your ErrorMessage
                        */

                        if (return_value == TRUE)
                        {
                           ErrorMessage = _T("No Error");
                           LastSentenceIDParsed = response_p->Mnemonic;
                           TalkerID = talker_id( sentence);
                           ExpandedTalkerID = expand_talker_id( TalkerID);
                        }
                        else
                        {
                           ErrorMessage = response_p->ErrorMessage;
                        }

                        break;
                   }

              node = node->GetNext();
        }

   }
   else
   {
      return_value = FALSE;
   }

   return( return_value);
}

NMEA0183& NMEA0183::operator << (wxString & source)
{
//   ASSERT_VALID( this);

   sentence = source;

   return( *this);
}

NMEA0183& NMEA0183::operator >> (wxString& destination)
{
//   ASSERT_VALID( this);

   destination = sentence;

   return( *this);
}
