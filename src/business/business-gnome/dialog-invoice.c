/*
 * dialog-invoice.c -- Dialog for Invoice entry
 * Copyright (C) 2001,2002,2006 Derek Atkins
 * Author: Derek Atkins <warlord@MIT.EDU>
 *
 * Copyright (c) 2005,2006 David Hampton <hampton@employees.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
 * Boston, MA  02110-1301,  USA       gnu@gnu.org
 */

#include "config.h"

#include <gnome.h>
#include <glib/gi18n.h>
#include <libguile.h>
#include "swig-runtime.h"

#include "qof.h"

#include "dialog-utils.h"
#include "gnc-component-manager.h"
#include "gnc-ui.h"
#include "gnc-gconf-utils.h"
#include "gnc-gui-query.h"
#include "gnc-ui-util.h"
#include "gnc-date-edit.h"
#include "gnc-amount-edit.h"
#include "gnucash-sheet.h"
#include "window-report.h"
#include "dialog-search.h"
#include "search-param.h"
#include "gnc-session.h"
#include "gncOwner.h"
#include "gncInvoice.h"
#include "gncInvoiceP.h"

#include "gncEntryLedger.h"

#include "gnc-plugin-page.h"
#include "gnc-general-search.h"
#include "dialog-date-close.h"
#include "dialog-invoice.h"
#include "dialog-job.h"
#include "business-gnome-utils.h"
#include "dialog-payment.h"
#include "dialog-tax-table.h"
#include "dialog-billterms.h"
#include "dialog-account.h"
#include "guile-mappings.h"

#include "dialog-query-list.h"

#include "gnc-plugin-business.h"
#include "gnc-plugin-page-invoice.h"
#include "gnc-main-window.h"

#include "dialog-transfer.h"

/* Disable -Waddress.  GCC 4.2 warns (and fails to compile with -Werror) when
 * passing the address of a guid on the stack to QOF_BOOK_LOOKUP_ENTITY via
 * gncInvoiceLookup and friends.  When the macro gets inlined, the compiler
 * emits a warning that the guid null pointer test is always true.
 */
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 2)
#    pragma GCC diagnostic warning "-Waddress"
#endif

#define DIALOG_NEW_INVOICE_CM_CLASS "dialog-new-invoice"
#define DIALOG_VIEW_INVOICE_CM_CLASS "dialog-view-invoice"

#define GCONF_SECTION_SEARCH  "dialogs/business/invoice_search"

#define LAST_POSTED_TO_ACCT "last-posted-to-acct"

void gnc_invoice_window_ok_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_window_cancel_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_window_help_cb (GtkWidget *widget, gpointer data);
void gnc_invoice_id_changed_cb (GtkWidget *widget, gpointer data);

#define ENUM_INVOICE_TYPE(_) \
  _(NEW_INVOICE, )  \
  _(MOD_INVOICE, )  \
  _(EDIT_INVOICE, ) \
  _(VIEW_INVOICE, )

DEFINE_ENUM(InvoiceDialogType, ENUM_INVOICE_TYPE)
AS_STRING_DEC(InvoiceDialogType, ENUM_INVOICE_TYPE)
FROM_STRING_DEC(InvoiceDialogType, ENUM_INVOICE_TYPE)

FROM_STRING_FUNC(InvoiceDialogType, ENUM_INVOICE_TYPE)
AS_STRING_FUNC(InvoiceDialogType, ENUM_INVOICE_TYPE)

struct _invoice_select_window
{
    QofBook *	book;
    GncOwner *	owner;
    QofQuery *	q;
    GncOwner	owner_def;
};


/** This data structure does double duty.  It is used to maintain
 *  information for the "New Invoice" dialog, and it is also used to
 *  maintain information for the "Invoice Entry" page that is embedded
 *  into a main window.  Beware, as not all fields are used by both windows.
 */
struct _invoice_window
{
    GladeXML   * xml;

    GtkWidget  * dialog;         /* Used by 'New Invoice Window' */
    GncPluginPage *page;        /* Used by 'Edit Invoice' Page */

    /* Summary Bar Widgets */
    GtkWidget  * total_label;
    GtkWidget  * total_cash_label;
    GtkWidget  * total_charge_label;
    GtkWidget  * total_subtotal_label;
    GtkWidget  * total_tax_label;

    /* Data Widgets */
    GtkWidget  * id_entry;
    GtkWidget  * notes_text;
    GtkWidget  * opened_date;
    GtkWidget  * posted_date_hbox;
    GtkWidget  * posted_date;
    GtkWidget  * active_check;

    GtkWidget  * owner_box;
    GtkWidget  * owner_label;
    GtkWidget  * owner_choice;
    GtkWidget  * job_label;
    GtkWidget  * job_box;
    GtkWidget  * job_choice;
    GtkWidget  * billing_id_entry;
    GtkWidget  * terms_menu;

    /* Project Widgets (used for Bills only) */
    GtkWidget  * proj_frame;
    GtkWidget  * proj_cust_box;
    GtkWidget  * proj_cust_choice;
    GtkWidget  * proj_job_box;
    GtkWidget  * proj_job_choice;

    /* Expense Voucher Widgets */
    GtkWidget  * to_charge_frame;
    GtkWidget  * to_charge_edit;

    gint         width;

    GncBillTerm     * terms;
    GnucashRegister * reg;
    GncEntryLedger  * ledger;

    invoice_sort_type_t last_sort;

    InvoiceDialogType   dialog_type;
    GncGUID      invoice_guid;
    gint         component_id;
    QofBook    * book;
    GncInvoice * created_invoice;
    GncOwner     owner;
    GncOwner     job;

    GncOwner     proj_cust;
    GncOwner     proj_job;

    /* for Unposting */
    gboolean     reset_tax_tables;
};

/* Forward definitions for CB functions */
void gnc_invoice_window_closeCB (GtkWidget *widget, gpointer data);
void gnc_invoice_window_active_toggled_cb (GtkWidget *widget, gpointer data);
gboolean gnc_invoice_window_leave_notes_cb (GtkWidget *widget, GdkEventFocus *event, gpointer data);

#define INV_WIDTH_PREFIX "invoice_reg"
#define BILL_WIDTH_PREFIX "bill_reg"
#define VOUCHER_WIDTH_PREFIX "voucher_reg"

static void gnc_invoice_update_window (InvoiceWindow *iw, GtkWidget *widget);
static InvoiceWindow * gnc_ui_invoice_modify (GncInvoice *invoice);

/*******************************************************************************/
/* FUNCTIONS FOR ACCESSING DATA STRUCTURE FIELDS */

static GtkWidget *
iw_get_window (InvoiceWindow *iw)
{
    if (iw->page)
        return gnc_plugin_page_get_window(iw->page);
    return iw->dialog;
}

GtkWidget *
gnc_invoice_get_register(InvoiceWindow *iw)
{
    if (iw)
        return (GtkWidget *)iw->reg;
    return NULL;
}

/*******************************************************************************/
/* FUNCTIONS FOR UNPOSTING */

static gboolean
iw_ask_unpost (InvoiceWindow *iw)
{
    GtkWidget *dialog, *toggle, *pixmap;
    GladeXML *xml;
    gint response;
    char *s;

    xml = gnc_glade_xml_new ("invoice.glade", "Unpost Message Dialog");
    dialog = glade_xml_get_widget (xml, "Unpost Message Dialog");
    toggle = glade_xml_get_widget (xml, "yes_tt_reset");
    pixmap = glade_xml_get_widget (xml, "q_pixmap");

    gtk_window_set_transient_for (GTK_WINDOW(dialog),
                                  GTK_WINDOW(iw_get_window(iw)));

    iw->reset_tax_tables = FALSE;

    s = gnome_program_locate_file (NULL,
                                   GNOME_FILE_DOMAIN_PIXMAP,
                                   "gnome-question.png", TRUE, NULL);
    if (s)
    {
        pixmap = gtk_image_new_from_file(s);
        g_free(s);
    }

    gtk_widget_show_all(dialog);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK)
        iw->reset_tax_tables =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));

    gtk_widget_destroy(dialog);
    return (response == GTK_RESPONSE_OK);
}

/*******************************************************************************/
/* INVOICE WINDOW */

static GncInvoice *
iw_get_invoice (InvoiceWindow *iw)
{
    if (!iw)
        return NULL;

    return gncInvoiceLookup (iw->book, &iw->invoice_guid);
}

static void gnc_ui_to_invoice (InvoiceWindow *iw, GncInvoice *invoice)
{
    GtkTextBuffer* text_buffer;
    GtkTextIter start, end;
    gchar *text;
    Timespec ts;

    if (iw->dialog_type == VIEW_INVOICE)
        return;

    gnc_suspend_gui_refresh ();

    gncInvoiceBeginEdit (invoice);

    if (iw->active_check)
        gncInvoiceSetActive (invoice, gtk_toggle_button_get_active
                             (GTK_TOGGLE_BUTTON (iw->active_check)));

    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(iw->notes_text));
    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    text = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
    gncInvoiceSetNotes (invoice, text);

    if (iw->to_charge_edit)
        gncInvoiceSetToChargeAmount (invoice,
                                     gnc_amount_edit_get_amount
                                     (GNC_AMOUNT_EDIT (iw->to_charge_edit)));

    /* Only set these values for NEW/MOD INVOICE types */
    if (iw->dialog_type != EDIT_INVOICE)
    {
        gncInvoiceSetID (invoice, gtk_editable_get_chars
                         (GTK_EDITABLE (iw->id_entry), 0, -1));
        gncInvoiceSetBillingID (invoice, gtk_editable_get_chars
                                (GTK_EDITABLE (iw->billing_id_entry), 0, -1));
        gncInvoiceSetTerms (invoice, iw->terms);

        ts = gnc_date_edit_get_date_ts (GNC_DATE_EDIT (iw->opened_date));
        gncInvoiceSetDateOpened (invoice, ts);

        gnc_owner_get_owner (iw->owner_choice, &(iw->owner));
        if (iw->job_choice)
            gnc_owner_get_owner (iw->job_choice, &(iw->job));

        /* Only set the job if we've actually got one */
        if (gncOwnerGetJob (&(iw->job)))
            gncInvoiceSetOwner (invoice, &(iw->job));
        else
            gncInvoiceSetOwner (invoice, &(iw->owner));

        /* Set the invoice currency based on the owner */
        gncInvoiceSetCurrency (invoice, gncOwnerGetCurrency (&iw->owner));

        /* Only set the BillTo if we've actually got one */
        if (gncOwnerGetJob (&iw->proj_job))
            gncInvoiceSetBillTo (invoice, &iw->proj_job);
        else
            gncInvoiceSetBillTo (invoice, &iw->proj_cust);
    }

    gncInvoiceCommitEdit (invoice);
    gnc_resume_gui_refresh ();
}

static gboolean
gnc_invoice_window_verify_ok (InvoiceWindow *iw)
{
    const char *res;
    gchar *string;

    /* save the current entry in the ledger? */
    if (!gnc_entry_ledger_check_close (iw_get_window(iw), iw->ledger))
        return FALSE;

    /* Check the Owner */
    gnc_owner_get_owner (iw->owner_choice, &(iw->owner));
    res = gncOwnerGetName (&(iw->owner));
    if (res == NULL || safe_strcmp (res, "") == 0)
    {
        gnc_error_dialog (iw_get_window(iw), "%s",
                          /* Translators: In this context,
                           * 'Billing information' maps to the
                           * label in the frame and means
                           * e.g. customer i.e. the company being
                           * invoiced. */
                          _("You need to supply Billing Information."));
        return FALSE;
    }

    /* Check the ID; set one if necessary */
    res = gtk_entry_get_text (GTK_ENTRY (iw->id_entry));
    if (safe_strcmp (res, "") == 0)
    {
        /* Invoices and bills have separate counters.
           Therefore we pass the GncOwer to gncInvoiceNextID
           so it knows whether we are creating a bill
           or an invoice. */
        string = gncInvoiceNextID(iw->book, &(iw->owner));
        gtk_entry_set_text (GTK_ENTRY (iw->id_entry), string);
        g_free(string);
    }

    return TRUE;
}

static gboolean
gnc_invoice_window_ok_save (InvoiceWindow *iw)
{
    if (!gnc_invoice_window_verify_ok (iw))
        return FALSE;

    {
        GncInvoice *invoice = iw_get_invoice (iw);
        if (invoice)
        {
            gnc_ui_to_invoice (iw, invoice);
        }
        /* Save the invoice to return it later. */
        iw->created_invoice = invoice;
    }
    return TRUE;
}

void
gnc_invoice_window_ok_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!gnc_invoice_window_ok_save (iw))
        return;

    /* Ok, we don't need this anymore */
    iw->invoice_guid = *guid_null ();

    /* if this is a NEW_INVOICE, and created_invoice is NON-NULL, then
     * open up a new window with the invoice.  This used to be done
     * in gnc_ui_invoice_new() but cannot be done anymore
     */
    if (iw->dialog_type == NEW_INVOICE && iw->created_invoice)
        gnc_ui_invoice_edit (iw->created_invoice);

    gnc_close_gui_component (iw->component_id);
}

void
gnc_invoice_window_cancel_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    gnc_close_gui_component (iw->component_id);
}

void
gnc_invoice_window_help_cb (GtkWidget *widget, gpointer data)
{
    gnc_gnome_help(HF_HELP, HL_USAGE);
}

void
gnc_invoice_window_destroy_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice (iw);

    gnc_suspend_gui_refresh ();

    if (iw->dialog_type == NEW_INVOICE && invoice != NULL)
    {
        gncInvoiceBeginEdit (invoice);
        gncInvoiceDestroy (invoice);
        iw->invoice_guid = *guid_null ();
    }

    gnc_entry_ledger_destroy (iw->ledger);
    gnc_unregister_gui_component (iw->component_id);
    gtk_widget_destroy(widget);
    gnc_resume_gui_refresh ();

    g_free (iw);
}

void
gnc_invoice_window_closeCB (GtkWidget *widget, gpointer data)
{
    gnc_invoice_window_ok_cb (widget, data);
}

void
gnc_invoice_window_editCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice (iw);

    if (invoice)
        gnc_ui_invoice_modify (invoice);
}

void
gnc_invoice_window_duplicateInvoiceCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice (iw);

    if (invoice)
        gnc_ui_invoice_duplicate (invoice);
}

void gnc_invoice_window_entryUpCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    if (!iw || !iw->ledger)
        return;

    gnc_entry_ledger_move_current_entry_updown(iw->ledger, TRUE);
}
void gnc_invoice_window_entryDownCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    if (!iw || !iw->ledger)
        return;

    gnc_entry_ledger_move_current_entry_updown(iw->ledger, FALSE);
}

void
gnc_invoice_window_recordCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw || !iw->ledger)
        return;

    if (!gnc_entry_ledger_commit_entry (iw->ledger))
        return;

    gnucash_register_goto_next_virt_row (iw->reg);
}

void
gnc_invoice_window_cancelCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw || !iw->ledger)
        return;

    gnc_entry_ledger_cancel_cursor_changes (iw->ledger);
}

void
gnc_invoice_window_deleteCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncEntry *entry;

    if (!iw || !iw->ledger)
        return;

    /* get the current entry based on cursor position */
    entry = gnc_entry_ledger_get_current_entry (iw->ledger);
    if (!entry)
    {
        gnc_entry_ledger_cancel_cursor_changes (iw->ledger);
        return;
    }

    /* deleting the blank entry just cancels */
    if (entry == gnc_entry_ledger_get_blank_entry (iw->ledger))
    {
        gnc_entry_ledger_cancel_cursor_changes (iw->ledger);
        return;
    }

    /* Verify that the user really wants to delete this entry */
    {
        const char *message = _("Are you sure you want to delete the "
                                "selected entry?");
        const char *order_warn = _("This entry is attached to an order and "
                                   "will be deleted from that as well!");
        char *msg;
        gboolean result;

        if (gncEntryGetOrder (entry))
            msg = g_strconcat (message, "\n\n", order_warn, (char *)NULL);
        else
            msg = g_strdup (message);

        result = gnc_verify_dialog (iw_get_window(iw), FALSE, "%s", msg);
        g_free (msg);

        if (!result)
            return;
    }

    /* Yep, let's delete */
    gnc_entry_ledger_delete_current_entry (iw->ledger);
    return;
}

void
gnc_invoice_window_duplicateCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw || !iw->ledger)
        return;

    gnc_entry_ledger_duplicate_current_entry (iw->ledger);
}

void
gnc_invoice_window_blankCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw || !iw->ledger)
        return;

    if (!gnc_entry_ledger_commit_entry (iw->ledger))
        return;

    {
        VirtualCellLocation vcell_loc;
        GncEntry *blank;

        blank = gnc_entry_ledger_get_blank_entry (iw->ledger);
        if (blank == NULL)
            return;

        if (gnc_entry_ledger_get_entry_virt_loc (iw->ledger, blank, &vcell_loc))
            gnucash_register_goto_virt_cell (iw->reg, vcell_loc);
    }
}

void
gnc_invoice_window_printCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice (iw);
    SCM func, arg;
    SCM args = SCM_EOL;
    int report_id;

    g_return_if_fail (invoice);

    func = scm_c_eval_string ("gnc:invoice-report-create");
    g_return_if_fail (scm_is_procedure (func));

    arg = SWIG_NewPointerObj(invoice, SWIG_TypeQuery("_p__gncInvoice"), 0);
    args = scm_cons (arg, args);

    /* scm_gc_protect_object(func); */

    arg = scm_apply (func, args, SCM_EOL);
    g_return_if_fail (scm_is_exact (arg));
    report_id = scm_num2int (arg, SCM_ARG1, G_STRFUNC);

    /* scm_gc_unprotect_object(func); */
    if (report_id >= 0)
        reportWindow (report_id);
}

void
gnc_invoice_window_postCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice;
    char *message, *memo, *ddue_label, *post_label, *acct_label, *question_label;
    Account *acc = NULL;
    GList * acct_types = NULL;
    GList * acct_commodities = NULL;
    Timespec ddue, postdate;
    gboolean accumulate;
    QofInstance *owner_inst;
    KvpFrame *kvpf;
    KvpValue *kvp_val;
    const char *text;
    EntryList *entries;
    GncEntry* entry;
    gboolean reverse;
    gboolean show_dialog = TRUE;

    /* Make sure the invoice is ok */
    if (!gnc_invoice_window_verify_ok (iw))
        return;

    invoice = iw_get_invoice (iw);
    if (!invoice)
        return;

    /* Check that there is at least one Entry */
    if (gncInvoiceGetEntries (invoice) == NULL)
    {
        gnc_error_dialog (iw_get_window(iw), "%s",
                          _("The Invoice must have at least one Entry."));
        return;
    }

    reverse = (gncInvoiceGetOwnerType (invoice) == GNC_OWNER_CUSTOMER);

    /* Make sure that the invoice has a positive balance */
    if (gnc_numeric_negative_p(gncInvoiceGetTotal(invoice)))
    {
        gnc_error_dialog(iw_get_window(iw), "%s",
                         _("You may not post an invoice with a negative total value."));
        return;
    }

    if (iw->total_cash_label &&
            gnc_numeric_negative_p(gncInvoiceGetTotalOf(invoice, GNC_PAYMENT_CASH)))
    {
        gnc_error_dialog(iw_get_window(iw), "%s",
                         _("You may not post an expense voucher with a negative total cash value."));
        return;
    }

    /* Ok, we can post this invoice.  Ask for verification, set the due date,
     * post date, and posted account
     */
    message = _("Do you really want to post the invoice?");
    ddue_label = _("Due Date");
    post_label = _("Post Date");
    acct_label = _("Post to Account");
    question_label = _("Accumulate Splits?");

    /* Determine the type of account to post to */
    acct_types = gncOwnerGetAccountTypesList (&(iw->owner));

    /* Determine which commodity we're working with */
    acct_commodities = gncOwnerGetCommoditiesList(&(iw->owner));

    /* Get the due date and posted account */
    postdate = gncInvoiceGetDateOpened (invoice);
    ddue = postdate;
    memo = NULL;

    owner_inst = qofOwnerGetOwner (gncOwnerGetEndOwner (&(iw->owner)));
    kvpf = qof_instance_get_slots (owner_inst);
    acc = xaccAccountLookup (kvp_frame_get_guid (kvpf, LAST_POSTED_TO_ACCT),
                             iw->book);

    /* Get the default for the accumulate option */
    accumulate = gnc_gconf_get_bool(GCONF_SECTION_INVOICE, "accumulate_splits", NULL);

    if (!gnc_dialog_dates_acct_question_parented (iw_get_window(iw), message, ddue_label,
            post_label, acct_label, question_label, TRUE, TRUE,
            acct_types, acct_commodities, iw->book, iw->terms,
            &ddue, &postdate, &memo, &acc, &accumulate))
        return;

    /* Yep, we're posting.  So, save the invoice...
     * Note that we can safely ignore the return value; we checked
     * the verify_ok earlier, so we know it's ok.
     */
    gnc_suspend_gui_refresh ();
    gncInvoiceBeginEdit (invoice);
    gnc_invoice_window_ok_save (iw);

    /* Fill in the conversion prices with feedback from the user */
    text = _("One or more of the entries are for accounts different from the invoice/bill currency.  You will be asked a conversion rate for each.");

    for (entries = gncInvoiceGetEntries(invoice); entries != NULL; entries = g_list_next(entries))
    {
        Account *this_acc;

        entry = (GncEntry*)entries->data;
        this_acc = (reverse ? gncEntryGetInvAccount (entry) :
                    gncEntryGetBillAccount (entry));

        if (!gnc_commodity_equal(gncInvoiceGetCurrency (invoice), xaccAccountGetCommodity(this_acc)))
        {
            GNCPrice *convprice;

            if (show_dialog)
            {
                gnc_info_dialog(iw_get_window(iw), "%s", text);
                show_dialog = FALSE;
            }

            convprice = gncInvoiceGetPrice(invoice, xaccAccountGetCommodity(this_acc));
            if (convprice == NULL)
            {
                XferDialog *xfer;
                gnc_numeric exch_rate;
                Timespec date;
                gnc_numeric amount = gnc_numeric_create(1, 1);


                /* create the exchange-rate dialog */
                xfer = gnc_xfer_dialog (iw_get_window(iw), this_acc);
                gnc_xfer_dialog_select_to_account(xfer, acc);
                gnc_xfer_dialog_set_amount(xfer, amount);

                /* All we want is the exchange rate so prevent the user from thinking
                   it makes sense to mess with other stuff */
                gnc_xfer_dialog_set_from_show_button_active(xfer, FALSE);
                gnc_xfer_dialog_set_to_show_button_active(xfer, FALSE);
                gnc_xfer_dialog_hide_from_account_tree(xfer);
                gnc_xfer_dialog_hide_to_account_tree(xfer);
                gnc_xfer_dialog_is_exchange_dialog(xfer, &exch_rate);
                gnc_xfer_dialog_run_until_done(xfer);

                convprice = gnc_price_create(iw->book);
                gnc_price_begin_edit (convprice);
                gnc_price_set_commodity (convprice, xaccAccountGetCommodity(this_acc));
                gnc_price_set_currency (convprice, gncInvoiceGetCurrency (invoice));
                date.tv_sec = time (NULL);
                date.tv_nsec = 0;
                gnc_price_set_time (convprice, date);
                gnc_price_set_source (convprice, "user:invoice-post");

                /* Yes, magic strings are evil but I can't find any defined constants
                   for this..*/
                gnc_price_set_typestr (convprice, "last");
                gnc_price_set_value (convprice, exch_rate);
                gncInvoiceAddPrice(invoice, convprice);
                gnc_price_commit_edit (convprice);
            }
        }
    }


    /* Save acc as last used account in the kvp frame of the invoice owner */
    kvp_val = kvp_value_new_guid (qof_instance_get_guid (QOF_INSTANCE (acc)));;
    qof_begin_edit (owner_inst);
    kvp_frame_set_slot_nc (kvpf, LAST_POSTED_TO_ACCT, kvp_val);
    qof_instance_set_dirty (owner_inst);
    qof_commit_edit (owner_inst);

    /* ... post it; post date is set to now ... */
    gncInvoicePostToAccount (invoice, acc, &postdate, &ddue, memo, accumulate);
    gncInvoiceCommitEdit (invoice);
    gnc_resume_gui_refresh ();

    if (memo)
        g_free (memo);

    /* Reset the type; change to read-only! */
    iw->dialog_type = VIEW_INVOICE;
    gnc_entry_ledger_set_readonly (iw->ledger, TRUE);

    /* ... and redisplay here. */
    gnc_invoice_update_window (iw, NULL);
    gnc_table_refresh_gui (gnc_entry_ledger_get_table (iw->ledger), FALSE);
}

void
gnc_invoice_window_unpostCB (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice;
    gboolean result;

    invoice = iw_get_invoice (iw);
    if (!invoice)
        return;

    /* make sure the user REALLY wants to do this! */
    result = iw_ask_unpost(iw);
    if (!result) return;

    /* Attempt to unpost the invoice */
    gnc_suspend_gui_refresh ();
    result = gncInvoiceUnpost (invoice, iw->reset_tax_tables);
    gnc_resume_gui_refresh ();
    if (!result) return;

    /* if we get here, we succeeded in unposting -- reset the ledger and redisplay */
    iw->dialog_type = EDIT_INVOICE;
    gnc_entry_ledger_set_readonly (iw->ledger, FALSE);
    gnc_invoice_update_window (iw, NULL);
    gnc_table_refresh_gui (gnc_entry_ledger_get_table (iw->ledger), FALSE);
}

void gnc_invoice_window_cut_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    gnucash_register_cut_clipboard (iw->reg);
}

void gnc_invoice_window_copy_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    gnucash_register_copy_clipboard (iw->reg);
}

void gnc_invoice_window_paste_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    gnucash_register_paste_clipboard (iw->reg);
}

void gnc_invoice_window_new_invoice_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    if (gncOwnerGetJob (&iw->job))
    {
        gnc_ui_invoice_new (&iw->job, iw->book);
    }
    else
    {
        gnc_ui_invoice_new (&iw->owner, iw->book);
    }
}

void gnc_business_call_owner_report (GncOwner *owner, Account *acc)
{
    int id;
    SCM args;
    SCM func;
    SCM arg;

    g_return_if_fail (owner);

    args = SCM_EOL;

    func = scm_c_eval_string ("gnc:owner-report-create");
    g_return_if_fail (scm_is_procedure (func));

    if (acc)
    {
        swig_type_info * qtype = SWIG_TypeQuery("_p_Account");
        g_return_if_fail (qtype);

        arg = SWIG_NewPointerObj(acc, qtype, 0);
        g_return_if_fail (arg != SCM_UNDEFINED);
        args = scm_cons (arg, args);
    }
    else
    {
        args = scm_cons (SCM_BOOL_F, args);
    }

    arg = SWIG_NewPointerObj(owner, SWIG_TypeQuery("_p__gncOwner"), 0);
    g_return_if_fail (arg != SCM_UNDEFINED);
    args = scm_cons (arg, args);

    /* Apply the function to the args */
    arg = scm_apply (func, args, SCM_EOL);
    g_return_if_fail (scm_is_exact (arg));
    id = scm_num2int (arg, SCM_ARG1, G_STRFUNC);

    if (id >= 0)
        reportWindow (id);
}

void gnc_invoice_window_report_owner_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    gnc_business_call_owner_report (&iw->owner, NULL);
}

void gnc_invoice_window_payment_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice(iw);

    if (gncOwnerGetJob (&iw->job))
        gnc_ui_payment_new_with_invoice (&iw->job, iw->book, invoice);
    else
        gnc_ui_payment_new_with_invoice (&iw->owner, iw->book, invoice);
}

/* Sorting callbacks */

void
gnc_invoice_window_sort (InvoiceWindow *iw, invoice_sort_type_t sort_code)
{
    QofQuery *query = gnc_entry_ledger_get_query (iw->ledger);
    GSList *p1 = NULL, *p2 = NULL, *p3 = NULL, *standard;

    if (iw->last_sort == sort_code)
        return;

    standard = g_slist_prepend (NULL, QUERY_DEFAULT_SORT);

    switch (sort_code)
    {
    case BY_STANDARD:
        p1 = standard;
        break;
    case BY_DATE:
        p1 = g_slist_prepend (p1, ENTRY_DATE);
        p2 = standard;
        break;
    case BY_DATE_ENTERED:
        p1 = g_slist_prepend (p1, ENTRY_DATE_ENTERED);
        p2 = standard;
        break;
    case BY_DESC:
        p1 = g_slist_prepend (p1, ENTRY_DESC);
        p2 = standard;
        break;
    case BY_QTY:
        p1 = g_slist_prepend (p1, ENTRY_QTY);
        p2 = standard;
        break;
    case BY_PRICE:
        p1 = g_slist_prepend (p1, ((iw->owner.type == GNC_OWNER_CUSTOMER) ?
                                   ENTRY_IPRICE : ENTRY_BPRICE));
        p2 = standard;
        break;
    default:
        g_slist_free (standard);
        g_return_if_fail (FALSE);
    }

    qof_query_set_sort_order (query, p1, p2, p3);
    iw->last_sort = sort_code;
    gnc_entry_ledger_display_refresh (iw->ledger);
}

/* Window configuration callbacks */

void
gnc_invoice_window_active_toggled_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice(iw);

    if (!invoice) return;

    gncInvoiceSetActive (invoice,
                         gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

gboolean
gnc_invoice_window_leave_notes_cb (GtkWidget *widget, GdkEventFocus *event,
                                   gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice(iw);
    GtkTextBuffer* text_buffer;
    GtkTextIter start, end;
    gchar *text;

    if (!invoice) return FALSE;

    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(iw->notes_text));
    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    text = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
    gncInvoiceSetNotes (invoice, text);
    return FALSE;
}

static gboolean
gnc_invoice_window_leave_to_charge_cb (GtkWidget *widget, GdkEventFocus *event,
                                       gpointer data)
{
    gnc_amount_edit_evaluate (GNC_AMOUNT_EDIT (widget));
    return FALSE;
}

static void
gnc_invoice_window_changed_to_charge_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice = iw_get_invoice(iw);

    if (!invoice) return;

    gncInvoiceSetToChargeAmount (invoice, gnc_amount_edit_get_amount
                                 (GNC_AMOUNT_EDIT (widget)));
}

static GtkWidget *
add_summary_label (GtkWidget *summarybar, const char *label_str)
{
    GtkWidget *hbox;
    GtkWidget *label;

    hbox = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start (GTK_BOX(summarybar), hbox, FALSE, FALSE, 5);

    label = gtk_label_new (label_str);
    gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

    label = gtk_label_new ("");
    gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

    return label;
}

GtkWidget *
gnc_invoice_window_create_summary_bar (InvoiceWindow *iw)
{
    GtkWidget *summarybar;

    iw->total_label	    = NULL;
    iw->total_cash_label      = NULL;
    iw->total_charge_label    = NULL;
    iw->total_subtotal_label  = NULL;
    iw->total_tax_label       = NULL;

    summarybar = gtk_hbox_new (FALSE, 4);

    iw->total_label	    = add_summary_label (summarybar, _("Total:"));

    switch (gncOwnerGetType (&iw->owner))
    {
    case GNC_OWNER_CUSTOMER:
    case GNC_OWNER_VENDOR:
        iw->total_subtotal_label = add_summary_label (summarybar, _("Subtotal:"));
        iw->total_tax_label     = add_summary_label (summarybar, _("Tax:"));
        break;

    case GNC_OWNER_EMPLOYEE:
        iw->total_cash_label    = add_summary_label (summarybar, _("Total Cash:"));
        iw->total_charge_label  = add_summary_label (summarybar, _("Total Charge:"));
        break;

    default:
        break;
    }

    gtk_widget_show_all(summarybar);
    return summarybar;
}

static int
gnc_invoice_job_changed_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice *invoice;
    char const *msg = "";

    if (!iw)
        return FALSE;

    if (iw->dialog_type == VIEW_INVOICE)
        return FALSE;

    gnc_owner_get_owner (iw->job_choice, &(iw->job));
    invoice = iw_get_invoice (iw);

    if (iw->dialog_type == EDIT_INVOICE)
        return FALSE;

    msg = gncJobGetReference (gncOwnerGetJob (&(iw->job)));
    gtk_entry_set_text (GTK_ENTRY (iw->billing_id_entry), msg ? msg : "");

    return FALSE;

}

static GNCSearchWindow *
gnc_invoice_select_job_cb (gpointer jobp, gpointer user_data)
{
    GncJob *j = jobp;
    InvoiceWindow *iw = user_data;
    GncOwner owner, *ownerp;

    if (!iw) return NULL;

    if (j)
    {
        ownerp = gncJobGetOwner (j);
        gncOwnerCopy (ownerp, &owner);
    }
    else
        gncOwnerCopy (&(iw->owner), &owner);

    return gnc_job_search (j, &owner, iw->book);
}

static void
gnc_invoice_update_job_choice (InvoiceWindow *iw)
{
    if (iw->job_choice)
        gtk_container_remove (GTK_CONTAINER (iw->job_box), iw->job_choice);

    /* If we don't have a real owner, then we obviously can't have a job */
    if (iw->owner.owner.undefined == NULL)
    {
        iw->job_choice = NULL;

    }
    else
        switch (iw->dialog_type)
        {
        case VIEW_INVOICE:
        case EDIT_INVOICE:
            iw->job_choice =
                gnc_owner_edit_create (NULL, iw->job_box, iw->book, &(iw->job));
            break;
        case NEW_INVOICE:
        case MOD_INVOICE:
            iw->job_choice =
                gnc_general_search_new (GNC_JOB_MODULE_NAME, _("Select..."), TRUE,
                                        gnc_invoice_select_job_cb, iw, iw->book);

            gnc_general_search_set_selected (GNC_GENERAL_SEARCH (iw->job_choice),
                                             gncOwnerGetJob (&iw->job));
            gnc_general_search_allow_clear (GNC_GENERAL_SEARCH (iw->job_choice),
                                            TRUE);
            gtk_box_pack_start (GTK_BOX (iw->job_box), iw->job_choice,
                                TRUE, TRUE, 0);

            g_signal_connect (G_OBJECT (iw->job_choice), "changed",
                              G_CALLBACK (gnc_invoice_job_changed_cb), iw);
            break;
        }

    if (iw->job_choice)
        gtk_widget_show_all (iw->job_choice);
}

static GNCSearchWindow *
gnc_invoice_select_proj_job_cb (gpointer jobp, gpointer user_data)
{
    GncJob *j = jobp;
    InvoiceWindow *iw = user_data;
    GncOwner owner, *ownerp;

    if (!iw) return NULL;

    if (j)
    {
        ownerp = gncJobGetOwner (j);
        gncOwnerCopy (ownerp, &owner);
    }
    else
        gncOwnerCopy (&(iw->proj_cust), &owner);

    return gnc_job_search (j, &owner, iw->book);
}

static int
gnc_invoice_proj_job_changed_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;

    if (!iw)
        return FALSE;

    if (iw->dialog_type == VIEW_INVOICE)
        return FALSE;

    gnc_owner_get_owner (iw->proj_job_choice, &(iw->proj_job));
    return FALSE;
}

static void
gnc_invoice_update_proj_job (InvoiceWindow *iw)
{
    if (iw->proj_job_choice)
        gtk_container_remove (GTK_CONTAINER (iw->proj_job_box),
                              iw->proj_job_choice);

    switch (iw->dialog_type)
    {
    case VIEW_INVOICE:
    case EDIT_INVOICE:
        iw->proj_job_choice =
            gnc_owner_edit_create (NULL, iw->proj_job_box, iw->book, &(iw->job));
        break;
    case NEW_INVOICE:
    case MOD_INVOICE:
        if (iw->proj_cust.owner.undefined == NULL)
        {
            iw->proj_job_choice = NULL;
        }
        else
        {
            iw->proj_job_choice =
                gnc_general_search_new (GNC_JOB_MODULE_NAME, _("Select..."), TRUE,
                                        gnc_invoice_select_proj_job_cb, iw, iw->book);

            gnc_general_search_set_selected (GNC_GENERAL_SEARCH(iw->proj_job_choice),
                                             gncOwnerGetJob (&iw->proj_job));
            gnc_general_search_allow_clear (GNC_GENERAL_SEARCH (iw->proj_job_choice),
                                            TRUE);
            gtk_box_pack_start (GTK_BOX (iw->proj_job_box), iw->proj_job_choice,
                                TRUE, TRUE, 0);

            g_signal_connect (G_OBJECT (iw->proj_job_choice), "changed",
                              G_CALLBACK (gnc_invoice_proj_job_changed_cb), iw);
        }
        break;
    }

    if (iw->proj_job_choice)
        gtk_widget_show_all (iw->proj_job_choice);
}

static int
gnc_invoice_owner_changed_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncBillTerm *term = NULL;
    GncOwner owner;

    if (!iw)
        return FALSE;

    if (iw->dialog_type == VIEW_INVOICE)
        return FALSE;

    gncOwnerCopy (&(iw->owner), &owner);
    gnc_owner_get_owner (iw->owner_choice, &owner);

    /* If this owner really changed, then reset ourselves */
    if (!gncOwnerEqual (&owner, &(iw->owner)))
    {
        GncInvoice *invoice;

        gncOwnerCopy (&owner, &(iw->owner));
        gncOwnerInitJob (&(iw->job), NULL);
        invoice = iw_get_invoice (iw);
        gnc_entry_ledger_reset_query (iw->ledger);
    }

    if (iw->dialog_type == EDIT_INVOICE)
        return FALSE;

    switch (gncOwnerGetType (&(iw->owner)))
    {
    case GNC_OWNER_CUSTOMER:
        term = gncCustomerGetTerms (gncOwnerGetCustomer (&(iw->owner)));
        break;
    case GNC_OWNER_VENDOR:
        term = gncVendorGetTerms (gncOwnerGetVendor (&(iw->owner)));
        break;
    case GNC_OWNER_EMPLOYEE:
        term = NULL;
        break;
    default:
        g_warning ("Unknown owner type: %d\n", gncOwnerGetType (&(iw->owner)));
        break;
    }

    /* XXX: I'm not sure -- should we change the terms if this happens? */
    iw->terms = term;
    gnc_ui_optionmenu_set_value (iw->terms_menu, iw->terms);

    gnc_invoice_update_job_choice (iw);

    return FALSE;
}

static int
gnc_invoice_proj_cust_changed_cb (GtkWidget *widget, gpointer data)
{
    InvoiceWindow *iw = data;
    GncOwner owner;

    if (!iw)
        return FALSE;

    if (iw->dialog_type == VIEW_INVOICE)
        return FALSE;

    gncOwnerCopy (&(iw->proj_cust), &owner);
    gnc_owner_get_owner (iw->proj_cust_choice, &owner);

    /* If this owner really changed, then reset ourselves */
    if (!gncOwnerEqual (&owner, &(iw->proj_cust)))
    {
        gncOwnerCopy (&owner, &(iw->proj_cust));
        gncOwnerInitJob (&(iw->proj_job), NULL);
    }

    if (iw->dialog_type == EDIT_INVOICE)
        return FALSE;

    gnc_invoice_update_proj_job (iw);

    return FALSE;
}

static void
gnc_invoice_dialog_close_handler (gpointer user_data)
{
    InvoiceWindow *iw = user_data;

    if (iw)
    {
        gtk_widget_destroy (iw->dialog);
    }
}

static void
gnc_invoice_window_close_handler (gpointer user_data)
{
    InvoiceWindow *iw = user_data;

    if (iw)
    {
        gnc_main_window_close_page(iw->page);
        iw->page = NULL;
    }
}

static void
gnc_invoice_reset_total_label (GtkLabel *label, gnc_numeric amt, gnc_commodity *com)
{
    char string[256];

    amt = gnc_numeric_convert (amt, gnc_commodity_get_fraction(com), GNC_HOW_RND_ROUND_HALF_UP);
    xaccSPrintAmount (string, amt, gnc_default_print_info (TRUE));
    gtk_label_set_text (label, string);
}

static void
gnc_invoice_redraw_all_cb (GnucashRegister *g_reg, gpointer data)
{
    InvoiceWindow *iw = data;
    GncInvoice * invoice;
    gnc_commodity * currency;
    gnc_numeric amount, to_charge_amt = gnc_numeric_zero();

    if (!iw)
        return;

    //  if (iw)
    //    gnc_invoice_update_window (iw, NULL);

    invoice = iw_get_invoice (iw);
    if (!invoice)
        return;

    currency = gncInvoiceGetCurrency (invoice);

    if (iw->total_label)
    {
        amount = gncInvoiceGetTotal (invoice);
        gnc_invoice_reset_total_label (GTK_LABEL (iw->total_label), amount, currency);
    }

    if (iw->total_subtotal_label)
    {
        amount = gncInvoiceGetTotalSubtotal (invoice);
        gnc_invoice_reset_total_label (GTK_LABEL (iw->total_subtotal_label), amount, currency);
    }

    if (iw->total_tax_label)
    {
        amount = gncInvoiceGetTotalTax (invoice);
        gnc_invoice_reset_total_label (GTK_LABEL (iw->total_tax_label), amount, currency);
    }

    /* Deal with extra items for the expense voucher */

    if (iw->to_charge_edit)
    {
        gnc_amount_edit_evaluate (GNC_AMOUNT_EDIT (iw->to_charge_edit));
        to_charge_amt = gnc_amount_edit_get_amount(GNC_AMOUNT_EDIT(iw->to_charge_edit));
    }

    if (iw->total_cash_label)
    {
        amount = gncInvoiceGetTotalOf (invoice, GNC_PAYMENT_CASH);
        amount = gnc_numeric_sub (amount, to_charge_amt,
                                  gnc_commodity_get_fraction (currency), GNC_HOW_RND_ROUND_HALF_UP);
        gnc_invoice_reset_total_label (GTK_LABEL (iw->total_cash_label), amount, currency);
    }

    if (iw->total_charge_label)
    {
        amount = gncInvoiceGetTotalOf (invoice, GNC_PAYMENT_CARD);
        amount = gnc_numeric_add (amount, to_charge_amt,
                                  gnc_commodity_get_fraction (currency), GNC_HOW_RND_ROUND_HALF_UP);
        gnc_invoice_reset_total_label (GTK_LABEL (iw->total_charge_label), amount, currency);
    }
}

void
gnc_invoice_window_changed (InvoiceWindow *iw, GtkWidget *window)
{
    gnc_entry_ledger_set_parent(iw->ledger, window);
}

gchar *
gnc_invoice_get_help (InvoiceWindow *iw)
{
    if (!iw)
        return NULL;

    return gnc_table_get_help (gnc_entry_ledger_get_table (iw->ledger));
}

static void
gnc_invoice_window_refresh_handler (GHashTable *changes, gpointer user_data)
{
    InvoiceWindow *iw = user_data;
    const EventInfo *info;
    GncInvoice *invoice = iw_get_invoice (iw);
    GncOwner *owner;

    /* If there isn't an invoice behind us, close down */
    if (!invoice)
    {
        gnc_close_gui_component (iw->component_id);
        return;
    }

    /* Next, close if this is a destroy event */
    if (changes)
    {
        info = gnc_gui_get_entity_events (changes, &iw->invoice_guid);
        if (info && (info->event_mask & QOF_EVENT_DESTROY))
        {
            gnc_close_gui_component (iw->component_id);
            return;
        }
    }

    /* Check the owners, and see if they have changed */
    owner = gncInvoiceGetOwner (invoice);

    /* Copy the owner information into our window */
    gncOwnerCopy (gncOwnerGetEndOwner (owner), &(iw->owner));
    gncOwnerInitJob (&(iw->job), gncOwnerGetJob (owner));

    /* Copy the billto information into our window */
    owner = gncInvoiceGetBillTo (invoice);
    gncOwnerCopy (gncOwnerGetEndOwner (owner), &iw->proj_cust);
    gncOwnerInitJob (&iw->proj_job, gncOwnerGetJob (owner));

    /* Ok, NOW let's refresh ourselves */
    gnc_invoice_update_window (iw, NULL);
}

/** Update the various widgets in the window/page based upon the data
 *  in the InvoiceWindow data structure.
 *
 *  @param iw A pointer to the InvoiceWindow data structure.
 *
 *  @param widget If set, this is the widget that will be used for the
 *  call to gtk_widget_show_all().  This is needed at window/page
 *  creation time when all of the iw/page linkages haven't been set up
 *  yet.
 */
static void
gnc_invoice_update_window (InvoiceWindow *iw, GtkWidget *widget)
{
    GtkWidget *acct_entry;
    GncInvoice *invoice;
    gboolean is_posted = FALSE;
    gboolean can_unpost = FALSE;

    invoice = iw_get_invoice (iw);

    if (iw->owner_choice)
        gtk_container_remove (GTK_CONTAINER (iw->owner_box), iw->owner_choice);

    if (iw->proj_cust_choice)
        gtk_container_remove (GTK_CONTAINER (iw->proj_cust_box),
                              iw->proj_cust_choice);

    switch (iw->dialog_type)
    {
    case VIEW_INVOICE:
    case EDIT_INVOICE:
        iw->owner_choice =
            gnc_owner_edit_create (iw->owner_label, iw->owner_box, iw->book,
                                   &(iw->owner));
        iw->proj_cust_choice =
            gnc_owner_edit_create (NULL, iw->proj_cust_box, iw->book,
                                   &(iw->proj_cust));
        break;
    case NEW_INVOICE:
    case MOD_INVOICE:
        iw->owner_choice =
            gnc_owner_select_create (iw->owner_label, iw->owner_box, iw->book,
                                     &(iw->owner));
        iw->proj_cust_choice =
            gnc_owner_select_create (NULL, iw->proj_cust_box, iw->book,
                                     &(iw->proj_cust));

        g_signal_connect (G_OBJECT (iw->owner_choice), "changed",
                          G_CALLBACK (gnc_invoice_owner_changed_cb), iw);

        g_signal_connect (G_OBJECT (iw->proj_cust_choice), "changed",
                          G_CALLBACK (gnc_invoice_proj_cust_changed_cb), iw);

        break;
    }

    if (iw->owner_choice)
        gtk_widget_show_all (iw->owner_choice);
    if (iw->proj_cust_choice)
        gtk_widget_show_all (iw->proj_cust_choice);

    gnc_invoice_update_job_choice (iw);
    gnc_invoice_update_proj_job (iw);

    /* Hide the project frame for customer invoices */
    if (iw->owner.type == GNC_OWNER_CUSTOMER)
        gtk_widget_hide_all (iw->proj_frame);

    /* Hide the "job" label and entry for employee invoices */
    if (iw->owner.type == GNC_OWNER_EMPLOYEE)
    {
        gtk_widget_hide_all (iw->job_label);
        gtk_widget_hide_all (iw->job_box);
    }

    acct_entry = glade_xml_get_widget (iw->xml, "acct_entry");

    /* We know that "invoice" (and "owner") exist now */
    do
    {
        GtkTextBuffer* text_buffer;
        const char *string;
        gchar * tmp_string;
        Timespec ts, ts_zero = {0, 0};
        Account *acct;

        gtk_entry_set_text (GTK_ENTRY (iw->id_entry), gncInvoiceGetID (invoice));

        gtk_entry_set_text (GTK_ENTRY (iw->billing_id_entry),
                            gncInvoiceGetBillingID (invoice));

        string = gncInvoiceGetNotes (invoice);
        text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(iw->notes_text));
        gtk_text_buffer_set_text (text_buffer, string, -1);

        if (iw->active_check)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (iw->active_check),
                                          gncInvoiceGetActive (invoice));

        ts = gncInvoiceGetDateOpened (invoice);
        if (timespec_equal (&ts, &ts_zero))
        {
            gnc_date_edit_set_time (GNC_DATE_EDIT (iw->opened_date), time(NULL));
        }
        else
        {
            gnc_date_edit_set_time_ts (GNC_DATE_EDIT (iw->opened_date), ts);
        }

        /* fill in the terms menu */
        iw->terms = gncInvoiceGetTerms (invoice);
        gnc_ui_optionmenu_set_value (iw->terms_menu, iw->terms);

        /*
         * Next, figure out if we've been posted, and if so set the
         * appropriate bits of information.. Then work on hiding or
         * showing as necessary.
         */

        acct = gncInvoiceGetPostedAcc (invoice);
        if (!acct)
            break;

        /* Ok, it's definitely posted. Setup the 'posted-invoice' fields now */
        is_posted = TRUE;

        /* Can we unpost this invoice?
         * XXX: right now we always can, but there
         * may be times in the future when we cannot.
         */
        can_unpost = TRUE;

        ts = gncInvoiceGetDatePosted (invoice);
        gnc_date_edit_set_time_ts (GNC_DATE_EDIT (iw->posted_date), ts);

        tmp_string = gnc_account_get_full_name (acct);
        gtk_entry_set_text (GTK_ENTRY (acct_entry), tmp_string);
        g_free(tmp_string);

    }
    while (FALSE);

    gnc_invoice_id_changed_cb(NULL, iw);
    if (iw->dialog_type == NEW_INVOICE || iw->dialog_type == MOD_INVOICE)
    {
        if (widget)
            gtk_widget_show (widget);
        else
            gtk_widget_show (iw_get_window(iw));
        return;
    }

    /* Fill in the to_charge amount (only in VIEW/EDIT modes) */
    {
        gnc_numeric amount;

        amount = gncInvoiceGetToChargeAmount (invoice);
        gnc_amount_edit_set_amount (GNC_AMOUNT_EDIT (iw->to_charge_edit), amount);
    }

    /* Hide/show the appropriate widgets based on our posted/paid state */

    {
        GtkWidget *hide;

        if (is_posted == TRUE)
        {
            hide = glade_xml_get_widget (iw->xml, "hide3");
            gtk_widget_hide_all (hide);
            hide = glade_xml_get_widget (iw->xml, "hide4");
            gtk_widget_hide_all (hide);

        }
        else  			/* ! posted */
        {
            hide = glade_xml_get_widget (iw->xml, "posted_label");
            gtk_widget_hide_all (hide);
            gtk_widget_hide_all (iw->posted_date_hbox);

            hide = glade_xml_get_widget (iw->xml, "acct_label");
            gtk_widget_hide_all (hide);
            gtk_widget_hide_all (acct_entry);

            hide = glade_xml_get_widget (iw->xml, "hide1");
            gtk_widget_hide_all (hide);
            hide = glade_xml_get_widget (iw->xml, "hide2");
            gtk_widget_hide_all (hide);
        }
    }

    /* Set the toolbar widgets sensitivity */
    if (iw->page)
        gnc_plugin_page_invoice_update_menus(iw->page, is_posted, can_unpost);

    /* Set the to_charge widget */
    gtk_widget_set_sensitive (iw->to_charge_edit, !is_posted);

    /* Hide the to_charge frame for all non-employee invoices,
     * or set insensitive if the employee does not have a charge card
     */
    if (iw->owner.type == GNC_OWNER_EMPLOYEE)
    {
        if (!gncEmployeeGetCCard (gncOwnerGetEmployee(&iw->owner)))
            gtk_widget_set_sensitive (iw->to_charge_edit, FALSE);
    }
    else
    {
        gtk_widget_hide_all (iw->to_charge_frame);
    }

    if (is_posted)
    {
        //    GtkWidget *hide;

        /* Setup viewer for read-only access */
        /*
        gtk_widget_set_sensitive (iw->id_entry, FALSE);
        gtk_widget_set_sensitive (iw->terms_menu, FALSE);
        gtk_widget_set_sensitive (iw->notes_text, FALSE); *//* XXX: should notes remain writable? */
    }

    if (widget)
        gtk_widget_show (widget);
    else
        gtk_widget_show (iw_get_window(iw));
}

gchar *
gnc_invoice_get_title (InvoiceWindow *iw)
{
    char *wintitle = NULL;
    const char *id = NULL;

    if (!iw) return NULL;

    switch (gncOwnerGetType (&iw->owner))
    {
    case GNC_OWNER_CUSTOMER:
        switch (iw->dialog_type)
        {
        case NEW_INVOICE:
            wintitle = _("New Invoice");
            break;
        case MOD_INVOICE:
        case EDIT_INVOICE:
            wintitle = _("Edit Invoice");
            break;
        case VIEW_INVOICE:
            wintitle = _("View Invoice");
            break;
        }
        break;
    case GNC_OWNER_VENDOR:
        switch (iw->dialog_type)
        {
        case NEW_INVOICE:
            wintitle = _("New Bill");
            break;
        case MOD_INVOICE:
        case EDIT_INVOICE:
            wintitle = _("Edit Bill");
            break;
        case VIEW_INVOICE:
            wintitle = _("View Bill");
            break;
        }
        break;
    case GNC_OWNER_EMPLOYEE:
        switch (iw->dialog_type)
        {
        case NEW_INVOICE:
            wintitle = _("New Expense Voucher");
            break;
        case MOD_INVOICE:
        case EDIT_INVOICE:
            wintitle = _("Edit Expense Voucher");
            break;
        case VIEW_INVOICE:
            wintitle = _("View Expense Voucher");
            break;
        }
        break;
    default:
        break;
    }

    if (iw->id_entry)
        id = gtk_entry_get_text (GTK_ENTRY (iw->id_entry));
    if (id && *id)
        return g_strconcat (wintitle, " - ", id, (char *)NULL);
    return g_strdup (wintitle);
}

void
gnc_invoice_id_changed_cb (GtkWidget *unused, gpointer data)
{
    InvoiceWindow *iw = data;
    gchar *title;

    if (!iw) return;
    if (iw->page)
    {
        gnc_plugin_page_invoice_update_title (iw->page);
    }
    else
    {
        title = gnc_invoice_get_title (iw);
        gtk_window_set_title (GTK_WINDOW (iw->dialog), title);
        g_free (title);
    }
}


static gboolean
find_handler (gpointer find_data, gpointer user_data)
{
    const GncGUID *invoice_guid = find_data;
    InvoiceWindow *iw = user_data;

    return(iw && guid_equal(&iw->invoice_guid, invoice_guid));
}

static InvoiceWindow *
gnc_invoice_new_page (QofBook *bookp, InvoiceDialogType type,
                      GncInvoice *invoice, GncOwner *owner,
                      GncMainWindow *window)
{
    InvoiceWindow *iw;
    GncOwner *billto;
    GncPluginPage *new_page;

    g_assert (type != NEW_INVOICE && type != MOD_INVOICE);
    g_assert (invoice != NULL);

    /*
     * Find an existing window for this invoice.  If found, bring it to
     * the front.
     */
    if (invoice)
    {
        GncGUID invoice_guid;

        invoice_guid = *gncInvoiceGetGUID (invoice);
        iw = gnc_find_first_gui_component (DIALOG_VIEW_INVOICE_CM_CLASS,
                                           find_handler, &invoice_guid);
        if (iw)
        {
            gnc_main_window_display_page(iw->page);
            return(iw);
        }
    }

    /*
     * No existing invoice window found.  Build a new one.
     */
    iw = g_new0 (InvoiceWindow, 1);
    iw->book = bookp;
    iw->dialog_type = type;
    iw->invoice_guid = *gncInvoiceGetGUID (invoice);
    iw->width = -1;

    /* Save this for later */
    gncOwnerCopy (gncOwnerGetEndOwner (owner), &(iw->owner));
    gncOwnerInitJob (&(iw->job), gncOwnerGetJob (owner));

    billto = gncInvoiceGetBillTo (invoice);
    gncOwnerCopy (gncOwnerGetEndOwner (billto), &(iw->proj_cust));
    gncOwnerInitJob (&iw->proj_job, gncOwnerGetJob (billto));

    /* Now create the plugin page for this invoice and display it. */
    new_page = gnc_plugin_page_invoice_new (iw);
    if (window)
        gnc_plugin_page_set_use_new_window (new_page, FALSE);
    else
        window = gnc_plugin_business_get_window ();

    gnc_main_window_open_page (window, new_page);

    /* Initialize the summary bar */
    gnc_invoice_redraw_all_cb(iw->reg, iw);

    return iw;
}

#define KEY_INVOICE_TYPE        "InvoiceType"
#define KEY_INVOICE_GUID        "InvoiceGUID"
#define KEY_OWNER_TYPE          "OwnerType"
#define KEY_OWNER_GUID          "OwnerGUID"

GncPluginPage *
gnc_invoice_recreate_page (GncMainWindow *window,
                           GKeyFile *key_file,
                           const gchar *group_name)
{
    InvoiceWindow *iw;
    GError *error = NULL;
    char *tmp_string = NULL, *owner_type = NULL;
    InvoiceDialogType type;
    GncInvoice *invoice;
    GncGUID guid;
    QofBook *book;
    GncOwner owner = { 0 };

    /* Get Invoice Type */
    tmp_string = g_key_file_get_string(key_file, group_name,
                                       KEY_INVOICE_TYPE, &error);
    if (error)
    {
        g_warning("Error reading group %s key %s: %s.",
                  group_name, KEY_INVOICE_TYPE, error->message);
        goto give_up;
    }
    type = InvoiceDialogTypefromString(tmp_string);
    g_free(tmp_string);

    /* Get Invoice GncGUID */
    tmp_string = g_key_file_get_string(key_file, group_name,
                                       KEY_INVOICE_GUID, &error);
    if (error)
    {
        g_warning("Error reading group %s key %s: %s.",
                  group_name, KEY_INVOICE_GUID, error->message);
        goto give_up;
    }
    if (!string_to_guid(tmp_string, &guid))
    {
        g_warning("Invalid invoice guid: %s.", tmp_string);
        goto give_up;
    }
    book = gnc_get_current_book();
    invoice = gncInvoiceLookup(gnc_get_current_book(), &guid);
    if (invoice == NULL)
    {
        g_warning("Can't find invoice %s in current book.", tmp_string);
        goto give_up;
    }
    g_free(tmp_string);

    /* Get Owner Type */
    owner_type = g_key_file_get_string(key_file, group_name,
                                       KEY_OWNER_TYPE, &error);
    if (error)
    {
        g_warning("Error reading group %s key %s: %s.",
                  group_name, KEY_OWNER_TYPE, error->message);
        goto give_up;
    }

    /* Get Owner GncGUID */
    tmp_string = g_key_file_get_string(key_file, group_name,
                                       KEY_OWNER_GUID, &error);
    if (error)
    {
        g_warning("Error reading group %s key %s: %s.",
                  group_name, KEY_OWNER_GUID, error->message);
        goto give_up;
    }
    if (!string_to_guid(tmp_string, &guid))
    {
        g_warning("Invalid owner guid: %s.", tmp_string);
        goto give_up;
    }

    if (!gncOwnerGetOwnerFromTypeGuid(book, &owner, owner_type, &guid))
    {
        g_warning("Can't find owner %s in current book.", tmp_string);
        goto give_up;
    }
    g_free(tmp_string);
    g_free(owner_type);

    iw = gnc_invoice_new_page (book, type, invoice, &owner, window);
    return iw->page;

give_up:
    g_warning("Giving up on restoring '%s'.", group_name);
    if (error)
        g_error_free(error);
    if (tmp_string)
        g_free(tmp_string);
    if (owner_type)
        g_free(owner_type);
    return NULL;
}

void
gnc_invoice_save_page (InvoiceWindow *iw,
                       GKeyFile *key_file,
                       const gchar *group_name)
{
    g_key_file_set_string(key_file, group_name, KEY_INVOICE_TYPE,
                          InvoiceDialogTypeasString(iw->dialog_type));
    g_key_file_set_string(key_file, group_name, KEY_INVOICE_GUID,
                          guid_to_string(&iw->invoice_guid));

    g_key_file_set_string(key_file, group_name, KEY_OWNER_TYPE,
                          qofOwnerGetType(&iw->owner));
    g_key_file_set_string(key_file, group_name, KEY_OWNER_GUID,
                          guid_to_string(gncOwnerGetGUID(&iw->owner)));
}

GtkWidget *
gnc_invoice_create_page (InvoiceWindow *iw, gpointer page)
{
    GncInvoice *invoice;
    GladeXML *xml;
    GtkWidget *dialog, *hbox;
    GncEntryLedger *entry_ledger = NULL;
    GncOwnerType owner_type;
    GncEntryLedgerType ledger_type;
    const gchar *gconf_section = NULL;

    invoice = gncInvoiceLookup(iw->book, &iw->invoice_guid);

    iw->page = page;

    /* Find the dialog */
    iw->xml = xml = gnc_glade_xml_new ("invoice.glade", "invoice_entry_vbox");
    dialog = glade_xml_get_widget (xml, "invoice_entry_vbox");

    /* Autoconnect all the signals */
    glade_xml_signal_autoconnect_full (xml, gnc_glade_autoconnect_full_func, iw);

    /* Grab the widgets */
    iw->id_entry = glade_xml_get_widget (xml, "id_entry");
    iw->billing_id_entry = glade_xml_get_widget (xml, "billing_id_entry");
    iw->terms_menu = glade_xml_get_widget (xml, "terms_menu");
    iw->notes_text = glade_xml_get_widget (xml, "notes_text");
    iw->active_check = glade_xml_get_widget (xml, "active_check");
    iw->owner_box = glade_xml_get_widget (xml, "owner_hbox");
    iw->owner_label = glade_xml_get_widget (xml, "owner_label");
    iw->job_label = glade_xml_get_widget (xml, "job_label");
    iw->job_box = glade_xml_get_widget (xml, "job_hbox");

    /* grab the project widgets */
    iw->proj_frame = glade_xml_get_widget (xml, "proj_frame");
    iw->proj_cust_box = glade_xml_get_widget (xml, "proj_cust_hbox");
    iw->proj_job_box = glade_xml_get_widget (xml, "proj_job_hbox");

    /* grab the to_charge widgets */
    {
        GtkWidget *edit;

        gnc_commodity *currency = gncInvoiceGetCurrency (invoice);
        GNCPrintAmountInfo print_info;

        iw->to_charge_frame = glade_xml_get_widget (xml, "to_charge_frame");
        edit = gnc_amount_edit_new();
        print_info = gnc_commodity_print_info (currency, FALSE);
        gnc_amount_edit_set_evaluate_on_enter (GNC_AMOUNT_EDIT (edit), TRUE);
        gnc_amount_edit_set_print_info (GNC_AMOUNT_EDIT (edit), print_info);
        gnc_amount_edit_set_fraction (GNC_AMOUNT_EDIT (edit),
                                      gnc_commodity_get_fraction (currency));
        iw->to_charge_edit = edit;
        gtk_widget_show (edit);
        hbox = glade_xml_get_widget (xml, "to_charge_box");
        gtk_box_pack_start (GTK_BOX (hbox), edit, TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(gnc_amount_edit_gtk_entry(GNC_AMOUNT_EDIT(edit))),
                         "focus-out-event",
                         G_CALLBACK(gnc_invoice_window_leave_to_charge_cb), iw);
        g_signal_connect(G_OBJECT(edit), "amount_changed",
                         G_CALLBACK(gnc_invoice_window_changed_to_charge_cb), iw);
    }

    hbox = glade_xml_get_widget (xml, "date_opened_hbox");
    iw->opened_date = gnc_date_edit_new (time(NULL), FALSE, FALSE);
    gtk_widget_show(iw->opened_date);
    gtk_box_pack_start (GTK_BOX(hbox), iw->opened_date, TRUE, TRUE, 0);

    iw->posted_date_hbox = glade_xml_get_widget (xml, "date_posted_hbox");
    iw->posted_date = gnc_date_edit_new (time(NULL), FALSE, FALSE);
    gtk_widget_show(iw->posted_date);
    gtk_box_pack_start (GTK_BOX(iw->posted_date_hbox), iw->posted_date,
                        TRUE, TRUE, 0);

    /* Make the opened and posted dates insensitive in this window */
    gtk_widget_set_sensitive (iw->opened_date, FALSE);
    gtk_widget_set_sensitive (iw->posted_date, FALSE);

    /* Build the ledger */
    ledger_type = GNCENTRY_INVOICE_VIEWER;
    owner_type = gncOwnerGetType (&iw->owner);
    switch (iw->dialog_type)
    {
    case EDIT_INVOICE:
        switch (owner_type)
        {
        case GNC_OWNER_CUSTOMER:
            ledger_type = GNCENTRY_INVOICE_ENTRY;
            break;
        case GNC_OWNER_VENDOR:
            ledger_type = GNCENTRY_BILL_ENTRY;
            break;
        case GNC_OWNER_EMPLOYEE:
            ledger_type = GNCENTRY_EXPVOUCHER_ENTRY;
            break;
        default:
            g_warning ("Invalid owner type");
        }
        break;
    case VIEW_INVOICE:
    default:
        switch (owner_type)
        {
        case GNC_OWNER_CUSTOMER:
            ledger_type = GNCENTRY_INVOICE_VIEWER;
            gconf_section = GCONF_SECTION_INVOICE;
            break;
        case GNC_OWNER_VENDOR:
            ledger_type = GNCENTRY_BILL_VIEWER;
            gconf_section = GCONF_SECTION_BILL;
            break;
        case GNC_OWNER_EMPLOYEE:
            ledger_type = GNCENTRY_EXPVOUCHER_VIEWER;
            gconf_section = GCONF_SECTION_BILL;
            break;
        default:
            g_warning ("Invalid owner type");
        }
    }
    entry_ledger = gnc_entry_ledger_new (iw->book, ledger_type);

    /* Save the ledger... */
    iw->ledger = entry_ledger;
    /* window will be updated in a callback */

    /* Set the entry_ledger's invoice */
    gnc_entry_ledger_set_default_invoice (entry_ledger, invoice);

    /* Set the gconf section */
    gnc_entry_ledger_set_gconf_section (entry_ledger, gconf_section);

    /* Setup initial values */
    iw->component_id =
        gnc_register_gui_component (DIALOG_VIEW_INVOICE_CM_CLASS,
                                    gnc_invoice_window_refresh_handler,
                                    gnc_invoice_window_close_handler,
                                    iw);

    gnc_gui_component_watch_entity_type (iw->component_id,
                                         GNC_INVOICE_MODULE_NAME,
                                         QOF_EVENT_MODIFY | QOF_EVENT_DESTROY);

    /* Create the register */
    {
        GtkWidget *regWidget, *frame, *window;

        /* Watch the order of operations, here... */
        regWidget = gnucash_register_new (gnc_entry_ledger_get_table
                                          (entry_ledger));
        gtk_widget_show(regWidget);
        gnc_table_init_gui( regWidget, entry_ledger );

        frame = glade_xml_get_widget (xml, "ledger_frame");
        gtk_container_add (GTK_CONTAINER (frame), regWidget);

        iw->reg = GNUCASH_REGISTER (regWidget);
        window = gnc_plugin_page_get_window(iw->page);
        GNUCASH_SHEET (iw->reg->sheet)->window = window;

        g_signal_connect (G_OBJECT (regWidget), "activate_cursor",
                          G_CALLBACK (gnc_invoice_window_recordCB), iw);
        g_signal_connect (G_OBJECT (regWidget), "redraw_all",
                          G_CALLBACK (gnc_invoice_redraw_all_cb), iw);
    }

    gnc_table_realize_gui (gnc_entry_ledger_get_table (entry_ledger));

    /* Now fill in a lot of the pieces and display properly */
    gnc_ui_billterms_optionmenu (iw->terms_menu, iw->book, TRUE, &iw->terms);
    gnc_invoice_update_window (iw, dialog);

    gnc_table_refresh_gui (gnc_entry_ledger_get_table (iw->ledger), TRUE);

    /* Show the dialog */
    //  gtk_widget_show_all (dialog);

    return dialog;
}

static InvoiceWindow *
gnc_invoice_window_new_invoice (QofBook *bookp, GncOwner *owner,
                                GncInvoice *invoice)
{
    InvoiceWindow *iw;
    GladeXML *xml;
    GtkWidget *hbox;
    GncOwner *billto;

    if (invoice)
    {
        /*
         * Try to find an existing window for this invoice.  If found,
         * bring it to the front.
         */
        GncGUID invoice_guid;

        invoice_guid = *gncInvoiceGetGUID (invoice);
        iw = gnc_find_first_gui_component (DIALOG_NEW_INVOICE_CM_CLASS,
                                           find_handler, &invoice_guid);
        if (iw)
        {
            gtk_window_present (GTK_WINDOW(iw->dialog));
            return(iw);
        }
    }

    /*
     * No existing invoice window found.  Build a new one.
     */

    iw = g_new0 (InvoiceWindow, 1);

    if (invoice == NULL)
    {
        iw->dialog_type = NEW_INVOICE;
        invoice = gncInvoiceCreate (bookp);
        gncInvoiceSetCurrency (invoice, gnc_default_currency ());
        iw->book = bookp;
    }
    else
    {
        iw->dialog_type = MOD_INVOICE;
        owner = gncInvoiceGetOwner (invoice);
        iw->book = gncInvoiceGetBook (invoice);
    }

    /* Save this for later */
    gncOwnerCopy (gncOwnerGetEndOwner(owner), &(iw->owner));
    gncOwnerInitJob (&(iw->job), gncOwnerGetJob (owner));

    billto = gncInvoiceGetBillTo (invoice);
    gncOwnerCopy (gncOwnerGetEndOwner (billto), &(iw->proj_cust));
    gncOwnerInitJob (&iw->proj_job, gncOwnerGetJob (billto));

    /* Find the glade page layout */
    iw->xml = xml = gnc_glade_xml_new ("invoice.glade", "New Invoice Dialog");
    iw->dialog = glade_xml_get_widget (xml, "New Invoice Dialog");

    g_object_set_data (G_OBJECT (iw->dialog), "dialog_info", iw);

    /* Grab the widgets */
    iw->id_entry = glade_xml_get_widget (xml, "id_entry");
    iw->billing_id_entry = glade_xml_get_widget (xml, "billing_id_entry");
    iw->terms_menu = glade_xml_get_widget (xml, "terms_menu");
    iw->notes_text = glade_xml_get_widget (xml, "notes_text");
    iw->owner_box = glade_xml_get_widget (xml, "owner_hbox");
    iw->owner_label = glade_xml_get_widget (xml, "owner_label");
    iw->job_label = glade_xml_get_widget (xml, "job_label");
    iw->job_box = glade_xml_get_widget (xml, "job_hbox");

    /* Grab the project widgets */
    iw->proj_frame = glade_xml_get_widget (xml, "proj_frame");
    iw->proj_cust_box = glade_xml_get_widget (xml, "proj_cust_hbox");
    iw->proj_job_box = glade_xml_get_widget (xml, "proj_job_hbox");

    hbox = glade_xml_get_widget (xml, "date_opened_hbox");
    iw->opened_date = gnc_date_edit_new (time(NULL), FALSE, FALSE);
    gtk_widget_show(iw->opened_date);
    gtk_box_pack_start (GTK_BOX(hbox), iw->opened_date, TRUE, TRUE, 0);

    /* If this is a New Invoice, reset the Notes file to read/write */
    gtk_widget_set_sensitive (iw->notes_text, (iw->dialog_type == NEW_INVOICE));

    /* Setup signals */
    glade_xml_signal_autoconnect_full( xml,
                                       gnc_glade_autoconnect_full_func,
                                       iw);
    /* Setup initial values */
    iw->invoice_guid = *gncInvoiceGetGUID (invoice);

    iw->component_id =
        gnc_register_gui_component (DIALOG_NEW_INVOICE_CM_CLASS,
                                    gnc_invoice_window_refresh_handler,
                                    gnc_invoice_dialog_close_handler,
                                    iw);

    gnc_gui_component_watch_entity_type (iw->component_id,
                                         GNC_INVOICE_MODULE_NAME,
                                         QOF_EVENT_MODIFY | QOF_EVENT_DESTROY);

    /* Now fill in a lot of the pieces and display properly */
    gnc_ui_billterms_optionmenu (iw->terms_menu, iw->book, TRUE, &iw->terms);
    gnc_invoice_update_window (iw, iw->dialog);
    gnc_table_refresh_gui (gnc_entry_ledger_get_table (iw->ledger), TRUE);

    return iw;
}

InvoiceWindow *
gnc_ui_invoice_edit (GncInvoice *invoice)
{
    InvoiceWindow *iw;
    InvoiceDialogType type;

    if (!invoice) return NULL;

    /* Immutable once we've been posted */
    if (gncInvoiceGetPostedAcc (invoice))
        type = VIEW_INVOICE;
    else
        type = EDIT_INVOICE;

    iw = gnc_invoice_new_page (gncInvoiceGetBook(invoice), type,
                               invoice, gncInvoiceGetOwner (invoice), NULL);

    return iw;
}

static InvoiceWindow *
gnc_ui_invoice_modify (GncInvoice *invoice)
{
    InvoiceWindow *iw;
    if (!invoice) return NULL;

    iw = gnc_invoice_window_new_invoice (NULL, NULL, invoice);
    return iw;
}

static void
set_gncEntry_date(gpointer data, gpointer user_data)
{
    GncEntry *entry = data;
    const Timespec* new_date = user_data;
    //g_warning("Modifying date for entry with desc=\"%s\"", gncEntryGetDescription(entry));

    gncEntrySetDate(entry, *new_date);
    /*gncEntrySetDateEntered(entry, *new_date); - don't modify this
     * because apparently it defines the ordering of the entries,
     * which we don't want to change. */
}


InvoiceWindow * gnc_ui_invoice_duplicate (GncInvoice *old_invoice)
{
    InvoiceWindow *iw;
    GncInvoice *new_invoice = NULL;
    Timespec new_date;
    gchar *new_id;
    GList *node;

    g_assert(old_invoice);

    // Create a deep copy of the old invoice
    new_invoice = gncInvoiceCopy(old_invoice);

    // The new invoice is for sure active
    gncInvoiceSetActive(new_invoice, TRUE);

    // and unposted
    if (gncInvoiceIsPosted (new_invoice))
    {
        gboolean result = gncInvoiceUnpost(new_invoice, TRUE);
        if (!result)
        {
            g_warning("Oops, error when unposting the copied invoice; ignoring.");
        }
    }

    // Set a new id from the respective counter
    new_id = gncInvoiceNextID(gnc_get_current_book(),
                              gncInvoiceGetOwner(new_invoice));
    gncInvoiceSetID(new_invoice, new_id);
    g_free(new_id);

    // Modify the date to today
    timespecFromTime_t(&new_date, gnc_timet_get_today_start());
    gncInvoiceSetDateOpened(new_invoice, new_date);

    // Also modify the date of all entries to today
    //g_warning("We have %d entries", g_list_length(gncInvoiceGetEntries(new_invoice)));
    g_list_foreach(gncInvoiceGetEntries(new_invoice),
                   &set_gncEntry_date, &new_date);

    // Now open that newly created invoice in the "edit" window
    iw = gnc_ui_invoice_edit (new_invoice);

    // And also open the "properties" pop-up... however, changing the
    // invoice ID won't be copied over to the tab title even though
    // it's correctly copied into the invoice.
    iw = gnc_invoice_window_new_invoice (NULL, NULL, new_invoice);

    return iw;
}

InvoiceWindow *
gnc_ui_invoice_new (GncOwner *ownerp, QofBook *bookp)
{
    InvoiceWindow *iw;
    GncOwner owner;

    if (ownerp)
    {
        gncOwnerCopy (ownerp, &owner);
    }
    else
        gncOwnerInitCustomer (&owner, NULL); /* XXX: pass in the owner type? */

    /* Make sure required options exist */
    if (!bookp) return NULL;

    iw = gnc_invoice_window_new_invoice (bookp, &owner, NULL);

    return iw;
}

/* Functions for invoice selection widgets */

static void
edit_invoice_direct (gpointer invoice, gpointer user_data)
{
    g_return_if_fail (invoice);
    gnc_ui_invoice_edit (invoice);
}

static void
edit_invoice_cb (gpointer *invoice_p, gpointer user_data)
{
    g_return_if_fail (invoice_p && user_data);
    if (! *invoice_p)
        return;
    edit_invoice_direct (*invoice_p, user_data);
}

static void
pay_invoice_direct (gpointer inv, gpointer user_data)
{
    GncInvoice *invoice = inv;

    g_return_if_fail (invoice);
    gnc_ui_payment_new_with_invoice (gncInvoiceGetOwner (invoice),
                                     gncInvoiceGetBook (invoice), invoice);
}

static void
pay_invoice_cb (gpointer *invoice_p, gpointer user_data)
{
    g_return_if_fail (invoice_p && user_data);
    if (! *invoice_p)
        return;
    pay_invoice_direct (*invoice_p, user_data);
}

static void
duplicate_invoice_direct (gpointer invoice, gpointer user_data)
{
    g_return_if_fail (invoice);
    gnc_ui_invoice_duplicate (invoice);
}

static void
duplicate_invoice_cb (gpointer *invoice_p, gpointer user_data)
{
    g_return_if_fail (invoice_p && user_data);
    if (! *invoice_p)
        return;
    duplicate_invoice_direct (*invoice_p, user_data);
}

static gpointer
new_invoice_cb (gpointer user_data)
{
    struct _invoice_select_window *sw = user_data;
    InvoiceWindow *iw;

    g_return_val_if_fail (user_data, NULL);

    iw = gnc_ui_invoice_new (sw->owner, sw->book);
    return iw_get_invoice (iw);
}

static void
free_invoice_cb (gpointer user_data)
{
    struct _invoice_select_window *sw = user_data;

    g_return_if_fail (sw);

    qof_query_destroy (sw->q);
    g_free (sw);
}

GNCSearchWindow *
gnc_invoice_search (GncInvoice *start, GncOwner *owner, QofBook *book)
{
    QofIdType type = GNC_INVOICE_MODULE_NAME;
    struct _invoice_select_window *sw;
    QofQuery *q, *q2 = NULL;
    GncOwnerType owner_type = GNC_OWNER_CUSTOMER;
    static GList *inv_params = NULL, *bill_params = NULL, *emp_params = NULL, *params;
    static GList *columns = NULL;
    const gchar *title, *label;
    static GNCSearchCallbackButton *buttons;
    static GNCSearchCallbackButton inv_buttons[] =
    {
        { N_("View/Edit Invoice"), edit_invoice_cb},
        { N_("Process Payment"), pay_invoice_cb},
        { N_("Duplicate"), duplicate_invoice_cb},
        { NULL },
    };
    static GNCSearchCallbackButton bill_buttons[] =
    {
        { N_("View/Edit Bill"), edit_invoice_cb},
        { N_("Process Payment"), pay_invoice_cb},
        { N_("Duplicate"), duplicate_invoice_cb},
        { NULL },
    };
    static GNCSearchCallbackButton emp_buttons[] =
    {
        /* Translators: The terms 'Voucher' and 'Expense Voucher' are used
           interchangeably in gnucash and mean the same thing. */
        { N_("View/Edit Voucher"), edit_invoice_cb},
        { N_("Process Payment"), pay_invoice_cb},
        { N_("Duplicate"), duplicate_invoice_cb},
        { NULL },
    };

    g_return_val_if_fail (book, NULL);

    /* Build parameter list in reverse order */
    if (inv_params == NULL)
    {
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Invoice Owner"), NULL, type,
                                               INVOICE_OWNER, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Invoice Notes"), NULL, type,
                                               INVOICE_NOTES, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Billing ID"), NULL, type,
                                               INVOICE_BILLINGID, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Is Paid?"), NULL, type,
                                               INVOICE_IS_PAID, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Date Posted"), NULL, type,
                                               INVOICE_POSTED, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Is Posted?"), NULL, type,
                                               INVOICE_IS_POSTED, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Date Opened"), NULL, type,
                                               INVOICE_OPENED, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Due Date"), NULL, type,
                                               INVOICE_DUE, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Company Name "), NULL, type,
                                               INVOICE_OWNER, OWNER_PARENT,
                                               OWNER_NAME, NULL);
        inv_params = gnc_search_param_prepend (inv_params,
                                               _("Invoice ID"), NULL, type,
                                               INVOICE_ID, NULL);
    }
    if (bill_params == NULL)
    {
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Bill Owner"), NULL, type,
                                                INVOICE_OWNER, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Bill Notes"), NULL, type,
                                                INVOICE_NOTES, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Billing ID"), NULL, type,
                                                INVOICE_BILLINGID, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Is Paid?"), NULL, type,
                                                INVOICE_IS_PAID, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Date Posted"), NULL, type,
                                                INVOICE_POSTED, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Is Posted?"), NULL, type,
                                                INVOICE_IS_POSTED, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Date Opened"), NULL, type,
                                                INVOICE_OPENED, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Due Date"), NULL, type,
                                                INVOICE_DUE, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Company Name "), NULL, type,
                                                INVOICE_OWNER, OWNER_PARENT,
                                                OWNER_NAME, NULL);
        bill_params = gnc_search_param_prepend (bill_params,
                                                _("Bill ID"), NULL, type,
                                                INVOICE_ID, NULL);
    }
    if (emp_params == NULL)
    {
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Voucher Owner"), NULL, type,
                                               INVOICE_OWNER, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Voucher Notes"), NULL, type,
                                               INVOICE_NOTES, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Billing ID"), NULL, type,
                                               INVOICE_BILLINGID, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Is Paid?"), NULL, type,
                                               INVOICE_IS_PAID, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Date Posted"), NULL, type,
                                               INVOICE_POSTED, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Is Posted?"), NULL, type,
                                               INVOICE_IS_POSTED, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Date Opened"), NULL, type,
                                               INVOICE_OPENED, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Due Date"), NULL, type,
                                               INVOICE_DUE, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Employee Name"), NULL, type,
                                               INVOICE_OWNER, OWNER_PARENT,
                                               OWNER_NAME, NULL);
        emp_params = gnc_search_param_prepend (emp_params,
                                               _("Voucher ID"), NULL, type,
                                               INVOICE_ID, NULL);
    }

    /* Build the column list in reverse order */
    if (columns == NULL)
    {
        columns = gnc_search_param_prepend (columns, _("Billing ID"), NULL, type,
                                            INVOICE_BILLINGID, NULL);
        columns = gnc_search_param_prepend (columns, _("Type"), NULL, type,
                                            INVOICE_TYPE, NULL);
        columns = gnc_search_param_prepend_with_justify (columns, _("Paid"),
                  GTK_JUSTIFY_CENTER, NULL, type,
                  INVOICE_IS_PAID, NULL);
        columns = gnc_search_param_prepend (columns, _("Posted"), NULL, type,
                                            INVOICE_POSTED, NULL);
        columns = gnc_search_param_prepend (columns, _("Company"), NULL, type,
                                            INVOICE_OWNER, OWNER_PARENT,
                                            OWNER_NAME, NULL);
        columns = gnc_search_param_prepend (columns, _("Due"), NULL, type,
                                            INVOICE_DUE, NULL);
        columns = gnc_search_param_prepend (columns, _("Opened"), NULL, type,
                                            INVOICE_OPENED, NULL);
        columns = gnc_search_param_prepend (columns, _("Num"), NULL, type,
                                            INVOICE_ID, NULL);
    }

    /* Build the queries */
    q = qof_query_create_for (type);
    qof_query_set_book (q, book);

    /* If owner is supplied, limit all searches to invoices who's owner
     * or end-owner is the supplied owner!  Show all invoices by this
     * owner.  If a Job is supplied, search for all invoices for that
     * job, but if a Customer is supplied, search for all invoices owned
     * by that Customer or any of that Customer's Jobs.  In other words,
     * match on <supplied-owner's guid> == Invoice->Owner->GncGUID or
     * Invoice->owner->parentGUID.
     */
    if (owner)
    {
        GncOwner *tmp = owner;

        /* First, figure out the type of owner here.. */
        owner_type = gncOwnerGetType(owner);
        while (owner_type == GNC_OWNER_JOB)
        {
            tmp = gncOwnerGetEndOwner(tmp);
            owner_type = gncOwnerGetType(tmp);
        }

        /* Then if there's an actual owner add it to the query
         * and limit the search to this owner
         * If there's only a type, limit the search to this type.
         */
        if (gncOwnerGetGUID (owner))
        {
            q2 = qof_query_create ();
            qof_query_add_guid_match (q2, g_slist_prepend
                                      (g_slist_prepend (NULL, QOF_PARAM_GUID),
                                       INVOICE_OWNER),
                                      gncOwnerGetGUID (owner), QOF_QUERY_OR);

            qof_query_add_guid_match (q2, g_slist_prepend
                                      (g_slist_prepend (NULL, OWNER_PARENTG),
                                       INVOICE_OWNER),
                                      gncOwnerGetGUID (owner), QOF_QUERY_OR);

            qof_query_merge_in_place (q, q2, QOF_QUERY_AND);
            qof_query_destroy (q2);
            q2 = qof_query_copy (q);
        }
        else
        {
            QofQueryPredData *inv_type_pred;
            GSList *param_list = NULL;
            inv_type_pred = qof_query_string_predicate(QOF_COMPARE_EQUAL,
                            gncInvoiceGetTypeFromOwnerType(owner_type),
                            QOF_STRING_MATCH_NORMAL, FALSE);
            param_list = g_slist_prepend (param_list, INVOICE_TYPE);
            qof_query_add_term (q, param_list, inv_type_pred, QOF_QUERY_AND);
        }
    }

#if 0
    if (start)
    {
        if (q2 == NULL)
            q2 = qof_query_copy (q);

        qof_query_add_guid_match (q2, g_slist_prepend (NULL, QOF_PARAM_GUID),
                                  gncInvoiceGetGUID (start), QOF_QUERY_AND);
    }
#endif

    /* Launch select dialog and return the result */
    sw = g_new0 (struct _invoice_select_window, 1);

    if (owner)
    {
        gncOwnerCopy (owner, &(sw->owner_def));
        sw->owner = &(sw->owner_def);
    }
    sw->book = book;
    sw->q = q;

    switch (owner_type)
    {
    case GNC_OWNER_VENDOR:
        title = _("Find Bill");
        label = _("Bill");
        params = bill_params;
        buttons = bill_buttons;
        break;
    case GNC_OWNER_EMPLOYEE:
        title = _("Find Expense Voucher");
        label = _("Expense Voucher");
        params = emp_params;
        buttons = emp_buttons;
        break;
    default:
        title = _("Find Invoice");
        label = _("Invoice");
        params = inv_params;
        buttons = inv_buttons;
        break;
    }
    return gnc_search_dialog_create (type, title, params, columns, q, q2,
                                     buttons, NULL, new_invoice_cb,
                                     sw, free_invoice_cb, GCONF_SECTION_SEARCH,
                                     label);
}

GNCSearchWindow *
gnc_invoice_search_select (gpointer start, gpointer book)
{
    GncInvoice *i = start;
    GncOwner owner, *ownerp;

    if (!book) return NULL;

    if (i)
    {
        ownerp = gncInvoiceGetOwner (i);
        gncOwnerCopy (ownerp, &owner);
    }
    else
        gncOwnerInitCustomer (&owner, NULL); /* XXX */

    return gnc_invoice_search (start, NULL, book);
}

GNCSearchWindow *
gnc_invoice_search_edit (gpointer start, gpointer book)
{
    if (start)
        gnc_ui_invoice_edit (start);

    return NULL;
}

DialogQueryList *
gnc_invoice_show_bills_due (QofBook *book, double days_in_advance)
{
    QofIdType type = GNC_INVOICE_MODULE_NAME;
    Query *q;
    QofQueryPredData* pred_data;
    time_t end_date;
    GList *res;
    gint len;
    Timespec ts;
    static GList *param_list = NULL;
    static GNCDisplayListButton buttons[] =
    {
        { N_("View/Edit Bill"), edit_invoice_direct },
        { N_("Process Payment"), pay_invoice_direct },
        { NULL },
    };

    /* Create the param list (in reverse order) */
    if (param_list == NULL)
    {
        param_list = gnc_search_param_prepend (param_list, _("Amount"), NULL, type,
                                               INVOICE_POST_LOT, LOT_BALANCE, NULL);
        param_list = gnc_search_param_prepend (param_list, _("Company"), NULL, type,
                                               INVOICE_OWNER, OWNER_NAME, NULL);
        param_list = gnc_search_param_prepend (param_list, _("Due"), NULL, type,
                                               INVOICE_DUE, NULL);
    }

    /* Create the query to search for invoices; set the book */
    q = qof_query_create();
    qof_query_search_for(q, GNC_INVOICE_MODULE_NAME);
    qof_query_set_book (q, book);

    /* We want to find all invoices where:
     *      invoice -> is_posted == TRUE
     * AND  invoice -> lot -> is_closed? == FALSE
     * AND  invoice -> type != _("Invoice") // note: currently the translated form
     * AND  invoice -> due >= (today - days_in_advance)
     */

    qof_query_add_boolean_match (q, g_slist_prepend(NULL, INVOICE_IS_POSTED), TRUE,
                                 QOF_QUERY_AND);

    qof_query_add_boolean_match (q, g_slist_prepend(g_slist_prepend(NULL, LOT_IS_CLOSED),
                                 INVOICE_POST_LOT), FALSE, QOF_QUERY_AND);

    /* Bug#602091, #639365: The INVOICE_TYPE string unfortunately is
     * stored in translated form due to the usage of gncInvoiceGetType
     * for user-visible strings as well. Hence, as an exception we
     * must also search for the translated here even though it's an
     * internal flag. */
    pred_data = qof_query_string_predicate (QOF_COMPARE_NEQ, _("Invoice"),
                                            QOF_STRING_MATCH_NORMAL, FALSE);
    qof_query_add_term (q, g_slist_prepend(NULL, INVOICE_TYPE), pred_data, QOF_QUERY_AND);

    end_date = time(NULL);
    if (days_in_advance < 0)
        days_in_advance = 0;
    end_date += days_in_advance * 60 * 60 * 24;

    ts.tv_sec = (gint64) end_date;
    ts.tv_nsec = 0;
    pred_data = qof_query_date_predicate (QOF_COMPARE_LTE, QOF_DATE_MATCH_NORMAL, ts);
    qof_query_add_term (q, g_slist_prepend(NULL, INVOICE_DUE), pred_data, QOF_QUERY_AND);

    res = qof_query_run(q);
    len = g_list_length (res);
    if (!res || len <= 0)
    {
        qof_query_destroy(q);
        return NULL;
    }

    {
        gchar *message = g_strdup_printf
                         (/* Translators: %d is the number of bills due. This is a
	                         ngettext(3) message. */
                             ngettext("The following %d bill is due:",
                                      "The following %d bills are due:",
                                      len),
                             len);
        DialogQueryList *dialog =
            gnc_dialog_query_list_create(param_list, q,
                                         _("Due Bills Reminder"),
                                         message,
                                         TRUE, FALSE,
                                         buttons, NULL);
        g_free(message);
        qof_query_destroy(q);
        return dialog;
    }
}

void
gnc_invoice_remind_bills_due (void)
{
    QofBook *book;
    gint days;

    if (!gnc_current_session_exist()) return;
    book = qof_session_get_book(gnc_get_current_session());
    days = gnc_gconf_get_float(GCONF_SECTION_BILL, "days_in_advance", NULL);

    gnc_invoice_show_bills_due(book, days);
}

void
gnc_invoice_remind_bills_due_cb (void)
{
    if (!gnc_gconf_get_bool(GCONF_SECTION_BILL, "notify_when_due", NULL))
        return;

    gnc_invoice_remind_bills_due();
}

