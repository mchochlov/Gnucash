/********************************************************************
 * druid-commodity.h -- fancy importer for old Gnucash files        *
 *                       (GnuCash)                                  *
 * Copyright (C) 2000 Bill Gribble <grib@billgribble.com>           *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
 ********************************************************************/

#ifndef DRUID_COMMODITY_H
#define DRUID_COMMODITY_H

#include "gnc-commodity.h"
#include "gnc-engine.h"

typedef struct _commoditydruid CommodityDruid;

/* create/destroy commodity import druid */
CommodityDruid  * gnc_ui_commodity_druid_create(const char * filename);
void            gnc_ui_commodity_druid_destroy(CommodityDruid * d);

/* invoke import druid modally */
void            gnc_import_legacy_commodities(const char * filename);

/* book-opened callback hook */
void gnc_import_commodities(QofSession *session, gpointer data);

#endif

