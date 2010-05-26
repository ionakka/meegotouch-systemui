/* -*- Mode: C; indent-tabs-mode: s; c-basic-offset: 4; tab-width: 4 -*- */
/* vim:set et ai sw=4 ts=4 sts=4: tw=80 cino="(0,W2s,i2s,t0,l1,:0" */
/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of systemui.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/
#include "xchecker.h"
#include "QX11Info"

#define DEBUG
#include "../../src/debug.h"

XChecker::XChecker()
{
	Display *dpy = display();
    
    class_atom = XInternAtom(dpy, "WM_CLASS", False);
    name_atom = XInternAtom(dpy, "_NET_WM_NAME", False);
    name_atom2 = XInternAtom(dpy, "WM_NAME", False);
    xembed_atom = XInternAtom(dpy, "_XEMBED_INFO", False);
    pid_atom = XInternAtom(dpy, "_NET_WM_PID", False);
    trans_atom = XInternAtom(dpy, "WM_TRANSIENT_FOR", False);
    utf8_string_atom = XInternAtom(dpy, "UTF8_STRING", False);
    current_app_atom = XInternAtom(dpy, "_MB_CURRENT_APP_WINDOW", False);
    win_type_atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    wm_state_atom = XInternAtom(dpy, "_NET_WM_STATE", False);
    theme_atom = XInternAtom(dpy, "_MB_THEME", False);
    hildon_stack_atom = XInternAtom(dpy, "_HILDON_STACKABLE_WINDOW", False);
    portrait_support = XInternAtom(dpy, "_HILDON_PORTRAIT_MODE_SUPPORT", False);
    portrait_request = XInternAtom(dpy, "_HILDON_PORTRAIT_MODE_REQUEST", False);
    non_comp_atom = XInternAtom(dpy, "_HILDON_NON_COMPOSITED_WINDOW", False);
}

Display *
XChecker::display()
{
    Display *dpy;

    // Could be XOpenDisplay(NULL), but we already have a display.
    dpy = XOpenDisplay(NULL);
	//dpy = QX11Info::display (); 
    Q_ASSERT (dpy != NULL);
    
    return dpy;
}

char *
XChecker::get_atom_prop (
        Display     *dpy, 
        Window       w, 
        Atom         atom)
{ 
    Atom type;
    int format, rc;
    unsigned long items;
    unsigned long left;
    Atom *value;
	char *copy;

    rc = XGetWindowProperty (
            dpy, w, atom, 0, 1, False, XA_ATOM, &type, &format,
            &items, &left, (unsigned char**)&value);

    if (type != XA_ATOM || format == 0 || rc != Success) {
        copy = strdup("");
    } else {
        char *s = XGetAtomName(dpy, *value);
	    copy = strdup(s);
	    XFree(s);
    }
    
    return copy;
}

Window
XChecker::get_win_prop (
        Display *dpy, 
        Window   w, 
        Atom     atom)
{ 
    Atom type;
    int format, rc;
    unsigned long items;
    unsigned long left;
    Window *value;

    rc = XGetWindowProperty (dpy, w, atom, 0, 1, False, XA_WINDOW, &type, 
            &format, &items, &left, (unsigned char**)&value);

    if (type == None || rc != Success)
        return 0;

    return *value;
}

unsigned long 
XChecker::get_card_prop (
        Display *dpy, 
        Window w, 
        Atom atom)
{ 
    Atom type;
    int format, rc;
    unsigned long items;
    unsigned long left;
    unsigned long *value;

    rc = XGetWindowProperty (
            dpy, w, atom, 0, 1, False,
            XA_CARDINAL, &type, &format,
            &items, &left, (unsigned char**)&value);
    
    if (type == None || rc != Success)
        return 0;

    return *value;
}

long 
XChecker::get_int_prop (
        Display *dpy, 
        Window w, 
        Atom atom)
{ 
          Atom type;
          int format, rc;
          unsigned long items;
          unsigned long left;
          unsigned long *value;

          rc = XGetWindowProperty (dpy, w, atom, 0, 1, False,
                              XA_INTEGER, &type, &format,
                              &items, &left, (unsigned char**)&value);
          if (type == None || rc != Success)
            return -1;
          else
          {
            return *value;
          }
}

long 
XChecker::get_xembed_prop(Display *dpy, Window w)
{ 
          Atom type;
          int format, rc;
          unsigned long items;
          unsigned long left;
          unsigned long *value;

          rc = XGetWindowProperty (dpy, w, xembed_atom, 0, 2, False,
                              XA_CARDINAL, &type, &format,
                              &items, &left, (unsigned char**)&value);
          if (type == None || rc != Success)
            return -1;
          else
          {
	    long ret;
	    ret = value[1] & (1 << 0);
	    XFree(value);
            return ret;
          }
}

char *
XChecker::get_str_prop(Display *dpy, Window w, Atom atom)
{ 
          Atom type;
          int format, rc;
          unsigned long items;
          unsigned long left;
          char *value;

          rc = XGetWindowProperty (dpy, w, atom, 0, 200, False,
                              XA_STRING, &type, &format,
                              &items, &left, (unsigned char**)&value);
          if (type == None || rc != Success)
            return NULL;
          else
          {
            char *s = strdup((const char*)value);
	    XFree(value);
            return s;
          }
}

char *
XChecker::get_utf8_prop (
        Display *dpy, 
        Window w, 
        Atom atom)
{ 
          Atom type;
          int format, rc;
          unsigned long items;
          unsigned long left;
          char *value;

          rc = XGetWindowProperty (dpy, w, atom, 0, 200, False,
				   utf8_string_atom, &type, &format,
				   &items, &left, (unsigned char**)&value);
          if (type == None || rc != Success)
            return NULL;
          else
          {
            char *s = strdup((const char*)value);
	    XFree(value);
            return s;
          }
}

const char *
XChecker::get_map_state (
        int state)
{
	switch (state) {
		case IsUnmapped:
			return "IsUnmapped";
		case IsUnviewable:
			return "IsUnviewable";
		case IsViewable:
			return "IsViewable";
		default:
			return "";
	}
}

void 
XChecker::print_children (
        Display *dpy, 
        Window   WindowID, 
        int      level,
        int      nthWindow)
{
    QString indent;

	unsigned int n_children = 0;
	Window *child_l = NULL;
	Window root_ret, parent_ret;
	int i;
	char *wmclass;
    char *wmname, *wmname2;
    char *wmtype, *wmstate;
        unsigned long por_sup, por_req;
	Window trans_for;
	char buf[100], xembed_buf[50];
	XWindowAttributes attrs = { 0 };
	long xembed, hildon_stack, non_comp;

	XQueryTree(dpy, WindowID, &root_ret, &parent_ret, &child_l, &n_children);

	for (i = 0; i < level; ++i)
		indent += "    ";

	wmclass = get_str_prop(dpy, WindowID, class_atom);
	wmname = get_utf8_prop(dpy, WindowID, name_atom);
	wmname2 = get_str_prop(dpy, WindowID, name_atom2);
	wmtype = get_atom_prop(dpy, WindowID, win_type_atom);
	wmstate = get_atom_prop(dpy, WindowID, wm_state_atom);
	por_sup = get_card_prop(dpy, WindowID, portrait_support);
	por_req = get_card_prop(dpy, WindowID, portrait_request);
	trans_for = get_win_prop(dpy, WindowID, trans_atom);
	hildon_stack = get_int_prop(dpy, WindowID, hildon_stack_atom);
        non_comp = get_int_prop(dpy, WindowID, non_comp_atom);
	XGetWindowAttributes(dpy, WindowID, &attrs);

	if (trans_for)
		snprintf(buf, 100, "(transient for 0x%lx)", trans_for);
	else
		buf[0] = '\0';

	xembed = get_xembed_prop(dpy, WindowID);
	if (xembed != -1)
		snprintf(xembed_buf, 50, "(XEmbed %ld)", xembed);
	else
		xembed_buf[0] = '\0';

    QString windowName = (wmname ? wmname : "(none)");
    QString windowName2 = (wmname2 ? wmname2 : "(none)");
    windowName = windowName + "/" + windowName2;
    if (n_children == 0) {
        SYS_DEBUG ("%03d %s0x%lx %8s %12s %s", 
                nthWindow,
                SYS_STR(indent), 
                WindowID,
                get_map_state(attrs.map_state),
			    wmclass ? wmclass : "(none)",
                SYS_STR(windowName));
#if 0
		printf("0x%lx %s %s %s HStack:%ld [PID %lu] WM_CLASS:%s WM_NAME:%s "
                       "_NET_WM_NAME:%s %s %s %s %s %s %s %s %s\n",
			w, wmtype,
			wmstate, get_map_state(attrs.map_state),
			hildon_stack,
			get_card_prop(dpy, w, pid_atom),
			wmclass ? wmclass : "(none)",
			wmname2 ? wmname2 : "(none)",
			wmname ? wmname : "(none)",
			xembed_buf,
			buf, attrs.override_redirect ? "override-redirect": "",
			attrs.all_event_masks & SubstructureRedirectMask ?
			"Substr.Redirect":"",
			attrs.all_event_masks & SubstructureNotifyMask ?
			"Substr.Notify":"",
                        por_sup ? "por.sup":"",
                        por_req ? "por.req":"",
                        non_comp != -1 ? "non-comp.":"");
#endif
	} else {
        SYS_DEBUG ("%03d %s0x%lx %8s %12s %s", 
                nthWindow,
                SYS_STR(indent), 
                WindowID,
                get_map_state(attrs.map_state),
			    wmclass ? wmclass : "(none)",
                SYS_STR(windowName));
#if 0
		printf("0x%lx %s %s %s HStack:%ld [PID %lu] WM_CLASS:%s WM_NAME:%s "
                       "_NET_WM_NAME:%s %s %s %s %s %s %s %s %s (%u children):\n",
			w, wmtype, wmstate, get_map_state(attrs.map_state),
			hildon_stack,
			get_card_prop(dpy, w, pid_atom),
			wmclass ? wmclass : "(none)",
			wmname2 ? wmname2 : "(none)",
			wmname ? wmname : "(none)",
			xembed_buf,
			buf, attrs.override_redirect ? "override-redirect": "",
			attrs.all_event_masks & SubstructureRedirectMask ?
			"Substr.Redirect":"",
			attrs.all_event_masks & SubstructureNotifyMask ?
			"Substr.Notify":"",
                        por_sup ? "por.sup":"",
                        por_req ? "por.req":"",
                        non_comp != -1 ? "non-comp.":"",
			n_children);
#endif
		for (i = 0; i < n_children; ++i) {
            ++nthWindow;
			print_children(dpy, child_l[i], level + 1, nthWindow);
		}
		XFree(child_l);
	}
	free(wmclass);
	free(wmname);
	free(wmtype);
	free(wmstate);
}

bool
XChecker::check_window (
        Window                 WindowID,
        XChecker::RequestCode  OpCode)
{
    Display          *Display = QX11Info::display ();
    bool              retval = false;
    XWindowAttributes attrs = { 0 };

    SYS_DEBUG ("*** WindowID = 0x%lx", WindowID);

    if (WindowID == None) {
        switch (OpCode) {
            case CheckIsVisible:
                SYS_WARNING ("Window 0x%lx should be visible it is 'None'.",
                        WindowID);
                retval = false;
                break;

            case CheckIsInvisible:
                retval = true;
                break;
        }

        goto finalize;
    }

	if (!XGetWindowAttributes (Display, WindowID, &attrs)) {
        switch (OpCode) {
            case CheckIsVisible:
                SYS_WARNING ("Window 0x%lx should be visible but does not "
                        "exists.", WindowID);
                retval = false;
                break;

            case CheckIsInvisible:
                retval = true;
                break;
        }

        goto finalize;
    }

    switch (OpCode) {
        case CheckIsVisible:
            if (attrs.map_state == IsViewable) {
                retval = true;
            } else {
                SYS_WARNING ("Window 0x%lx should be visible, but it is not.",
                        WindowID);
            }
            break;

        case CheckIsInvisible:
            if (attrs.map_state != IsViewable) {
                retval = true;
            } else {
                SYS_WARNING ("Window 0x%lx should not be visible, but it is.",
                        WindowID);
            }
            break;
    }

finalize:
    if (!retval) 
        debug_dump_windows ();
    return retval;
}

void 
XChecker::debug_dump_windows()
{
	Display *dpy = display();
	Window root;

	root = XDefaultRootWindow(dpy);
	print_children(dpy, root, 0, 0);
}





