#include "cmdutil.h"

ScribusApp* Carrier;
ScribusDoc* doc;

double PointToValue(double Val)
{
	double ret = 0.0;
	switch (Carrier->doc->Einheit)
	{
	case 0:
		ret = Val;
		break;
	case 1:
		ret = Val * 0.3527777;
		break;
	case 2:
		ret = Val * (1.0 / 72.0);
		break;
	case 3:
		ret = Val * (1.0 / 12.0);
		break;
	}
	return ret;
}

double ValToPts(double Val, int ein)
{
	double ret = 0.0;
	switch (ein)
	{
	case 0:
		ret = Val;
		break;
	case 1:
		ret = Val / 0.3527777;
		break;
	case 2:
		ret = Val / (1.0 / 72.0);
		break;
	case 3:
		ret = Val / (1.0 / 12.0);
		break;
	}
	return ret;
}

double ValueToPoint(double Val)
{
	return ValToPts(Val, Carrier->doc->Einheit);
}

int GetItem(QString Name)
{
	if (Name != "")
	{
		for (uint a = 0; a < Carrier->doc->Items.count(); a++)
		{
			if (Carrier->doc->Items.at(a)->AnName == Name)
				return static_cast<int>(a);
		}
	}
	else
	{
		if (Carrier->view->SelItem.count() != 0)
			return Carrier->view->SelItem.at(0)->ItemNr;
	}
	return -1;
}

void ReplaceColor(QString col, QString rep)
{
	QColor tmpc;
	for (uint c = 0; c < Carrier->doc->Items.count(); c++)
	{
		PageItem *ite = Carrier->doc->Items.at(c);
		if (ite->PType == 4)
		{
			for (uint d = 0; d < ite->Ptext.count(); d++)
			{
				if (col == ite->Ptext.at(d)->ccolor)
					ite->Ptext.at(d)->ccolor = rep;
				if (col == ite->Ptext.at(d)->cstroke)
					ite->Ptext.at(d)->cstroke = rep;
			}
		}
		if (col == ite->Pcolor)
			ite->Pcolor = rep;
		if (col == ite->Pcolor2)
			ite->Pcolor2 = rep;
		QPtrVector<VColorStop> cstops = ite->fill_gradient.colorStops();
		for (uint cst = 0; cst < ite->fill_gradient.Stops(); ++cst)
		{
			if (col == cstops.at(cst)->name)
			{
				ite->SetFarbe(&tmpc, rep, cstops.at(cst)->shade);
				cstops.at(cst)->color = tmpc;
				cstops.at(cst)->name = rep;
			}
		}
	}
	for (uint c = 0; c < Carrier->doc->MasterItems.count(); c++)
	{
		PageItem *ite = Carrier->doc->MasterItems.at(c);
		if (ite->PType == 4)
		{
			for (uint d = 0; d < ite->Ptext.count(); d++)
			{
				if (col == ite->Ptext.at(d)->ccolor)
					ite->Ptext.at(d)->ccolor = rep;
				if (col == ite->Ptext.at(d)->cstroke)
					ite->Ptext.at(d)->cstroke = rep;
			}
		}
		if (col == ite->Pcolor)
			ite->Pcolor = rep;
		if (col == ite->Pcolor2)
			ite->Pcolor2 = rep;
		QPtrVector<VColorStop> cstops = ite->fill_gradient.colorStops();
		for (uint cst = 0; cst < ite->fill_gradient.Stops(); ++cst)
		{
			if (col == cstops.at(cst)->name)
			{
				ite->SetFarbe(&tmpc, rep, cstops.at(cst)->shade);
				cstops.at(cst)->color = tmpc;
				cstops.at(cst)->name = rep;
			}
		}
	}
}

/* 04/07/10 returns selection if is not name specified  pv  */
PageItem* GetUniqueItem(QString name)
{
	if (name.length()==0)
		if (Carrier->view->SelItem.count() != 0)
			return Carrier->view->SelItem.at(0);
		else
		{
			PyErr_SetString(ScribusException, QString("Can't use empty string for object name when there is no selection"));
			return NULL;
		}
	for (uint j = 0; j<Carrier->doc->Items.count(); j++)
	{
		if (name==Carrier->doc->Items.at(j)->AnName)
			return Carrier->doc->Items.at(j);
	} // for items
	PyErr_SetString(ScribusException, QString("Object not found"));
	return NULL;
}


/*!
 * Checks to see if a pageItem named 'name' exists and return true
 * if it does exist. Returns false if there is no such object, or
 * if the empty string ("") is passed.
 */
bool ItemExists(QString name)
{
	if (name.length() == 0)
		return false;
	for (uint j = 0; j<Carrier->doc->Items.count(); j++)
	{
		if (name==Carrier->doc->Items.at(j)->AnName)
			return true;
	} // for items
	return false;
}

/*!
 * Checks to see if there is a document open.
 * If there is an open document, returns true.
 * If there is no open document, sets a Python
 * exception and returns false.
 * 2004-10-27 Craig Ringer
 */
bool checkHaveDocument()
{
    if (Carrier->HaveDoc)
        return true;
    // Caller is required to check for false return from this function
    // and return NULL.
    PyErr_SetString(NoDocOpenError, QString("Command does not make sense without an open document"));
    return false;
}
