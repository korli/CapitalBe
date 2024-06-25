#include "CategoryBox.h"
#include <Catalog.h>
#include "CBLocale.h"
#include "Database.h"
#include "MsgDefs.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CategoryWindow"


CategoryBoxFilter::CategoryBoxFilter(CategoryBox* box)
	: AutoTextControlFilter(box)
{
}

filter_result
CategoryBoxFilter::KeyFilter(const int32& key, const int32& mod)
{
	// Here is where all the *real* work for a date box is done.
	if (key == B_TAB) {
		if (mod & B_SHIFT_KEY)
			SendMessage(new BMessage(M_PREVIOUS_FIELD));
		else
			SendMessage(new BMessage(M_NEXT_FIELD));
		return B_SKIP_MESSAGE;
	}

#ifdef ENTER_NAVIGATION
	if (key == B_ENTER) {
		SendMessage(new BMessage(M_ENTER_NAVIGATION));
		return B_SKIP_MESSAGE;
	}
#endif

	//	if(key == B_ESCAPE && !IsEscapeCancel())
	//		return B_SKIP_MESSAGE;

	if (key < 32
		|| ((mod & B_COMMAND_KEY) && !(mod & B_SHIFT_KEY) && !(mod & B_OPTION_KEY)
			&& !(mod & B_CONTROL_KEY)))
		return B_DISPATCH_MESSAGE;

	Account* acc = gDatabase.CurrentAccount();
	if (!acc)
		return B_DISPATCH_MESSAGE;

	int32 start, end;
	TextControl()->TextView()->GetSelection(&start, &end);
	if (end == (int32)strlen(TextControl()->Text())) {
		TextControl()->TextView()->Delete(start, end);

		BString string;
		if (GetCurrentMessage()->FindString("bytes", &string) != B_OK)
			string = "";
		string.Prepend(TextControl()->Text());

		BString autocomplete = acc->AutocompleteCategory(string.String());

		if (autocomplete.CountChars() > 0) {
			BMessage automsg(M_CATEGORY_AUTOCOMPLETE);
			automsg.AddInt32("start", strlen(TextControl()->Text()) + 1);
			automsg.AddString("string", autocomplete.String());
			SendMessage(&automsg);
		}
	}

	return B_DISPATCH_MESSAGE;
}

CategoryBox::CategoryBox(
	const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	: AutoTextControl(name, label, text, msg, flags)
{
	SetFilter(new CategoryBoxFilter(this));
	SetCharacterLimit(32);
}

bool
CategoryBox::Validate(void)
{
	BString category(Text());

	if (category == "") {
		DAlert* alert = new DAlert(B_TRANSLATE("Category is missing"),
			B_TRANSLATE("Do you really want to add this transaction without a category?\n\n"
				"Even then, you need to select a transaction type, 'income' or 'spending'."),
			B_TRANSLATE("Income"), B_TRANSLATE("Spending"), B_TRANSLATE("Cancel"),
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		int32 value = alert->Go();
		if (value == 0) {
			fType = "DEP";
			SetText(B_TRANSLATE("Uncategorized"));
			return true;
		}
		if (value == 1) {
			fType = "ATM";
			SetText(B_TRANSLATE("Uncategorized"));
			return true;
		}
		else
			return false;
	}
	CapitalizeEachWord(category);
	SetText(category);
	SetTypeFromCategory(category);
	return true;
}

void
CategoryBox::SetTypeFromCategory(BString category)
{
	CppSQLite3Query query = gDatabase.DBQuery(
		"SELECT * FROM categorylist ORDER BY name ASC", "CategoryView::CategoryView");
	while (!query.eof()) {
		category_type type = (category_type)query.getIntField(1);
		BString name = DeescapeIllegalCharacters(query.getStringField(0));

		if (name.ICompare(category) == 0) {
			type == SPENDING ? fType = "ATM" : fType = "DEP";
			break;
		}
		query.nextRow();
	}
}