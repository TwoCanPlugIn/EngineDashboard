/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - Window and Instrument container classes
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#ifndef _DASHBOARD_WINDOW_CONTAINER_H_
#define _DASHBOARD_WINDOW_CONTAINER_H_

#include "wx/wx.h"
#include "instrument.h"

class DashboardWindow;

WX_DEFINE_ARRAY(InstrumentProperties *, wxArrayOfInstrumentProperties);

//  DashboardWindowContainer
//  Holds metadata and ownership of one dashboard panel
class DashboardWindowContainer {
public:
    DashboardWindowContainer(DashboardWindow *dashboard_window,
                             wxString name, wxString caption,
                             wxString orientation, wxArrayInt inst,
                             wxArrayOfInstrumentProperties inProperty)
    {
        m_pDashboardWindow       = dashboard_window;
        m_sName                  = name;
        m_sCaption               = caption;
        m_sOrientation           = orientation;
        m_aInstrumentList        = inst;
        m_aInstrumentPropertyList= inProperty;
        m_bIsVisible             = false;
        m_bIsDeleted             = false;
    }

    ~DashboardWindowContainer() {
        for (unsigned int i = 0; i < m_aInstrumentPropertyList.GetCount(); i++) {
            InstrumentProperties *Inst = m_aInstrumentPropertyList.Item(i);
            delete Inst;
        }
    }

    DashboardWindow *m_pDashboardWindow;
    bool             m_bIsVisible;
    bool             m_bIsDeleted;
    bool             m_bPersVisible;  // Persists visibility across tool toggles
    wxString         m_sName;
    wxString         m_sCaption;
    wxString         m_sOrientation;
    wxArrayInt       m_aInstrumentList;
    wxArrayOfInstrumentProperties m_aInstrumentPropertyList;
    wxSize           m_best_size;
    wxSize           m_conf_best_size;
    wxSize           m_persist_size;
};


//  DashboardInstrumentContainer
//  Wraps one instrument widget with its ID and capability flags
class DashboardInstrumentContainer {
public:
    DashboardInstrumentContainer(int id, DashboardInstrument *instrument,
                                 CapType capa)
    {
        m_ID         = id;
        m_pInstrument= instrument;
        m_cap_flag   = capa;
    }
    ~DashboardInstrumentContainer() { delete m_pInstrument; }

    DashboardInstrument *m_pInstrument;
    int                  m_ID;
    CapType              m_cap_flag;
};

#ifdef __WX261
WX_DEFINE_ARRAY_PTR(DashboardWindowContainer *, wxArrayOfDashboard);
WX_DEFINE_ARRAY_PTR(DashboardInstrumentContainer *, wxArrayOfInstrument);
#else
WX_DEFINE_ARRAY(DashboardWindowContainer *, wxArrayOfDashboard);
WX_DEFINE_ARRAY(DashboardInstrumentContainer *, wxArrayOfInstrument);
#endif

#endif  // _DASHBOARD_WINDOW_CONTAINER_H_
