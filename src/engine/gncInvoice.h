/********************************************************************\
 * gncInvoice.h -- the Core Business Invoice Interface              *
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
 *                                                                  *
\********************************************************************/
/** @addtogroup Business
    @{ */
/** @addtogroup Invoice

An invoice holds a list of entries, a pointer to the customer,
and the job, the dates entered and posted, as well as the account,
transaction and lot for the posted invoice.
    @{ */
/** @file gncInvoice.h
    @brief  Business Invoice Interface
    @author Copyright (C) 2001,2006 Derek Atkins <warlord@MIT.EDU>
    @author Copyright (c) 2005 Neil Williams <linux@codehelp.co.uk>
*/

#ifndef GNC_INVOICE_H_
#define GNC_INVOICE_H_

struct _gncInvoice;
typedef struct _gncInvoice GncInvoice;
typedef struct _gncInvoiceClass GncInvoiceClass;
typedef GList GncInvoiceList;

#include "gncBillTerm.h"
#include "gncEntry.h"
#include "gncOwner.h"
#include "gnc-lot.h"
#include "qofbook.h"
#include "qofbook.h"
#include "gnc-pricedb.h"

#define GNC_ID_INVOICE    "gncInvoice"

/* --- type macros --- */
#define GNC_TYPE_INVOICE            (gnc_invoice_get_type ())
#define GNC_INVOICE(o)              \
     (G_TYPE_CHECK_INSTANCE_CAST ((o), GNC_TYPE_INVOICE, GncInvoice))
#define GNC_INVOICE_CLASS(k)        \
     (G_TYPE_CHECK_CLASS_CAST((k), GNC_TYPE_INVOICE, GncInvoiceClass))
#define GNC_IS_INVOICE(o)           \
     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNC_TYPE_INVOICE))
#define GNC_IS_INVOICE_CLASS(k)     \
     (G_TYPE_CHECK_CLASS_TYPE ((k), GNC_TYPE_INVOICE))
#define GNC_INVOICE_GET_CLASS(o)    \
     (G_TYPE_INSTANCE_GET_CLASS ((o), GNC_TYPE_INVOICE, GncInvoiceClass))
GType gnc_invoice_get_type(void);

/** @name Create/Destroy Functions
 @{ */
GncInvoice *gncInvoiceCreate (QofBook *book);

void gncInvoiceDestroy (GncInvoice *invoice);

/** Create a new GncInvoice object as a deep copy of the given other
 * invoice.
 *
 * The returned new invoice has everything copied from the other
 * invoice, including the ID string field. All GncEntries are newly
 * allocated copies of the original invoice's entries. */
GncInvoice *gncInvoiceCopy (const GncInvoice *other_invoice);
/** @} */

/** @name Set Functions
 @{ */
void gncInvoiceSetID (GncInvoice *invoice, const char *id);
void gncInvoiceSetOwner (GncInvoice *invoice, GncOwner *owner);
void gncInvoiceSetDateOpened (GncInvoice *invoice, Timespec date);
void gncInvoiceSetDatePosted (GncInvoice *invoice, Timespec date);
void gncInvoiceSetTerms (GncInvoice *invoice, GncBillTerm *terms);
void gncInvoiceSetBillingID (GncInvoice *invoice, const char *billing_id);
void gncInvoiceSetNotes (GncInvoice *invoice, const char *notes);
void gncInvoiceSetCurrency (GncInvoice *invoice, gnc_commodity *currency);
void gncInvoiceSetActive (GncInvoice *invoice, gboolean active);
void gncInvoiceSetBillTo (GncInvoice *invoice, GncOwner *billto);
void gncInvoiceSetToChargeAmount (GncInvoice *invoice, gnc_numeric amount);
/** @} */

void gncInvoiceAddEntry (GncInvoice *invoice, GncEntry *entry);
void gncInvoiceRemoveEntry (GncInvoice *invoice, GncEntry *entry);
void gncInvoiceAddPrice (GncInvoice *invoice, GNCPrice *price);
void gncInvoiceRemovePrice (GncInvoice *invoice, GNCPrice *price);

/** Call this function when adding an entry to a bill instead of an invoice */
void gncBillAddEntry (GncInvoice *bill, GncEntry *entry);
void gncBillRemoveEntry (GncInvoice *bill, GncEntry *entry);
void gncBillAddPrice (GncInvoice *bill, GNCPrice *price);
void gncBillRemovePrice (GncInvoice *bill, GNCPrice *price);

/** Call this function when an Entry is changed and you want to
    re-sort the list of entries
*/
void gncInvoiceSortEntries (GncInvoice *invoice);

/** @name Get Functions
 @{ */
const char * gncInvoiceGetID (const GncInvoice *invoice);
GncOwner * gncInvoiceGetOwner (GncInvoice *invoice);
Timespec gncInvoiceGetDateOpened (const GncInvoice *invoice);
Timespec gncInvoiceGetDatePosted (const GncInvoice *invoice);
Timespec gncInvoiceGetDateDue (const GncInvoice *invoice);
GncBillTerm * gncInvoiceGetTerms (const GncInvoice *invoice);
const char * gncInvoiceGetBillingID (const GncInvoice *invoice);
const char * gncInvoiceGetNotes (const GncInvoice *invoice);
GncOwnerType gncInvoiceGetOwnerType (GncInvoice *invoice);
const char * gncInvoiceGetTypeFromOwnerType (GncOwnerType type);
const char * gncInvoiceGetType (GncInvoice *invoice);
gnc_commodity * gncInvoiceGetCurrency (const GncInvoice *invoice);
GncOwner * gncInvoiceGetBillTo (GncInvoice *invoice);
gnc_numeric gncInvoiceGetToChargeAmount (const GncInvoice *invoice);
gboolean gncInvoiceGetActive (const GncInvoice *invoice);

GNCLot * gncInvoiceGetPostedLot (const GncInvoice *invoice);
Transaction * gncInvoiceGetPostedTxn (const GncInvoice *invoice);
Account * gncInvoiceGetPostedAcc (const GncInvoice *invoice);
/** @} */

/** return the "total" amount of the invoice */
gnc_numeric gncInvoiceGetTotal (GncInvoice *invoice);
gnc_numeric gncInvoiceGetTotalOf (GncInvoice *invoice, GncEntryPaymentType type);
gnc_numeric gncInvoiceGetTotalSubtotal (GncInvoice *invoice);
gnc_numeric gncInvoiceGetTotalTax (GncInvoice *invoice);

typedef GList EntryList;
EntryList * gncInvoiceGetEntries (GncInvoice *invoice);
GList * gncInvoiceGetPrices(GncInvoice *invoice);
GNCPrice * gncInvoiceGetPrice(GncInvoice *invoice, gnc_commodity* commodity);

/** Post this invoice to an account.  Returns the new Transaction
 * that is tied to this invoice.   The transaction is set with
 * the supplied posted date, due date, and memo.  The Transaction
 * description is set to the name of the company.
 */
Transaction *
gncInvoicePostToAccount (GncInvoice *invoice, Account *acc,
                         Timespec *posted_date, Timespec *due_date,
                         const char *memo, gboolean accumulatesplits);

/**
 * UNpost this invoice.  This will destroy the posted transaction and
 * return the invoice to its unposted state.  It may leave empty lots
 * out there.  If reset_tax_tables is TRUE, then it will also revert
 * all the Tax Tables to the parent, which will potentially change the
 * total value of the invoice.  It may also leave some orphaned Tax
 * Table children.
 *
 * Returns TRUE if successful, FALSE if there is a problem.
 */
gboolean
gncInvoiceUnpost (GncInvoice *invoice, gboolean reset_tax_tables);


/** Given a transaction, find and return the Invoice */
GncInvoice * gncInvoiceGetInvoiceFromTxn (const Transaction *txn);

/** Given a LOT, find and return the Invoice attached to the lot */
GncInvoice * gncInvoiceGetInvoiceFromLot (GNCLot *lot);

/** Return a pointer to the instance gncInvoice that is identified
 *  by the guid, and is residing in the book. Returns NULL if the
 *  instance can't be found.
 *  Equivalent function prototype is
 *  GncInvoice * gncInvoiceLookup (QofBook *book, const GncGUID *guid);
 */
static inline GncInvoice * gncInvoiceLookup (const QofBook *book, const GncGUID *guid)
{
    QOF_BOOK_RETURN_ENTITY(book, guid, GNC_ID_INVOICE, GncInvoice);
}

void gncInvoiceBeginEdit (GncInvoice *invoice);
void gncInvoiceCommitEdit (GncInvoice *invoice);
int gncInvoiceCompare (const GncInvoice *a, const GncInvoice *b);
gboolean gncInvoiceEqual(const GncInvoice *a, const GncInvoice *b);
gboolean gncInvoiceIsPosted (const GncInvoice *invoice);
gboolean gncInvoiceIsPaid (const GncInvoice *invoice);

#define INVOICE_ID          "id"
#define INVOICE_OWNER       "owner"
#define INVOICE_OPENED      "date_opened"
#define INVOICE_POSTED      "date_posted"
#define INVOICE_DUE         "date_due"
#define INVOICE_IS_POSTED   "is_posted?"
#define INVOICE_IS_PAID     "is_paid?"
#define INVOICE_TERMS       "terms"
#define INVOICE_BILLINGID   "billing_id"
#define INVOICE_NOTES       "notes"
#define INVOICE_ACC         "account"
#define INVOICE_POST_TXN    "posted_txn"
#define INVOICE_POST_LOT    "posted_lot"
#define INVOICE_TYPE        "type"
#define INVOICE_BILLTO      "bill-to"
#define INVOICE_ENTRIES     "list_of_entries"
#define INVOICE_JOB         "invoice_job"

#define INVOICE_FROM_LOT    "invoice-from-lot"
#define INVOICE_FROM_TXN    "invoice-from-txn"

QofBook *gncInvoiceGetBook(GncInvoice *x);

/** deprecated functions */
#define gncInvoiceGetGUID(x) qof_instance_get_guid(QOF_INSTANCE(x))
#define gncInvoiceRetGUID(x) (x ? *(qof_instance_get_guid(QOF_INSTANCE(x))) : *(guid_null()))
#define gncInvoiceLookupDirect(G,B) gncInvoiceLookup((B),&(G))

#endif /* GNC_INVOICE_H_ */
/** @} */
/** @} */
