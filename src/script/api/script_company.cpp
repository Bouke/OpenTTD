/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_company.cpp Implementation of ScriptCompany. */

#include "../../stdafx.h"
#include "script_company.hpp"
#include "script_error.hpp"
#include "script_companymode.hpp"
#include "../../company_func.h"
#include "../../company_base.h"
#include "../../company_manager_face.h"
#include "../../economy_func.h"
#include "../../object_type.h"
#include "../../strings_func.h"
#include "../../tile_map.h"
#include "../../string_func.h"
#include "../../settings_func.h"
#include "../../company_cmd.h"
#include "../../misc_cmd.h"
#include "../../object_cmd.h"
#include "../../settings_cmd.h"
#include "table/strings.h"

#include "../../safeguards.h"

/* static */ ScriptCompany::CompanyID ScriptCompany::ResolveCompanyID(ScriptCompany::CompanyID company)
{
	if (company == COMPANY_SELF) {
		if (!::Company::IsValidID(_current_company)) return COMPANY_INVALID;
		return (CompanyID)((byte)_current_company);
	}

	return ::Company::IsValidID(company) ? company : COMPANY_INVALID;
}

/* static */ bool ScriptCompany::IsMine(ScriptCompany::CompanyID company)
{
	EnforceCompanyModeValid(false);
	return ResolveCompanyID(company) == ResolveCompanyID(COMPANY_SELF);
}

/* static */ bool ScriptCompany::SetName(Text *name)
{
	CCountedPtr<Text> counter(name);

	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, name != nullptr);
	const std::string &text = name->GetDecodedText();
	EnforcePreconditionEncodedText(false, text);
	EnforcePreconditionCustomError(false, ::Utf8StringLength(text) < MAX_LENGTH_COMPANY_NAME_CHARS, ScriptError::ERR_PRECONDITION_STRING_TOO_LONG);

	return ScriptObject::Command<CMD_RENAME_COMPANY>::Do(text);
}

/* static */ std::optional<std::string> ScriptCompany::GetName(ScriptCompany::CompanyID company)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return std::nullopt;

	::SetDParam(0, company);
	return GetString(STR_COMPANY_NAME);
}

/* static */ bool ScriptCompany::SetPresidentName(Text *name)
{
	CCountedPtr<Text> counter(name);

	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, name != nullptr);
	const std::string &text = name->GetDecodedText();
	EnforcePreconditionEncodedText(false, text);
	EnforcePreconditionCustomError(false, ::Utf8StringLength(text) < MAX_LENGTH_PRESIDENT_NAME_CHARS, ScriptError::ERR_PRECONDITION_STRING_TOO_LONG);

	return ScriptObject::Command<CMD_RENAME_PRESIDENT>::Do(text);
}

/* static */ std::optional<std::string> ScriptCompany::GetPresidentName(ScriptCompany::CompanyID company)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return std::nullopt;

	::SetDParam(0, company);
	return GetString(STR_PRESIDENT_NAME);
}

/* static */ bool ScriptCompany::SetPresidentGender(Gender gender)
{
	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, gender == GENDER_MALE || gender == GENDER_FEMALE);
	EnforcePrecondition(false, GetPresidentGender(ScriptCompany::COMPANY_SELF) != gender);

	Randomizer &randomizer = ScriptObject::GetRandomizer();
	CompanyManagerFace cmf;
	GenderEthnicity ge = (GenderEthnicity)((gender == GENDER_FEMALE ? (1 << ::GENDER_FEMALE) : 0) | (randomizer.Next() & (1 << ETHNICITY_BLACK)));
	RandomCompanyManagerFaceBits(cmf, ge, false, randomizer);

	return ScriptObject::Command<CMD_SET_COMPANY_MANAGER_FACE>::Do(cmf);
}

/* static */ ScriptCompany::Gender ScriptCompany::GetPresidentGender(CompanyID company)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return GENDER_INVALID;

	GenderEthnicity ge = (GenderEthnicity)GetCompanyManagerFaceBits(Company::Get(company)->face, CMFV_GEN_ETHN, GE_WM);
	return HasBit(ge, ::GENDER_FEMALE) ? GENDER_FEMALE : GENDER_MALE;
}

/* static */ Money ScriptCompany::GetQuarterlyIncome(ScriptCompany::CompanyID company, SQInteger quarter)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return -1;
	if (quarter > EARLIEST_QUARTER) return -1;
	if (quarter < CURRENT_QUARTER) return -1;

	if (quarter == CURRENT_QUARTER) {
		return ::Company::Get(company)->cur_economy.income;
	}
	return ::Company::Get(company)->old_economy[quarter - 1].income;
}

/* static */ Money ScriptCompany::GetQuarterlyExpenses(ScriptCompany::CompanyID company, SQInteger quarter)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return -1;
	if (quarter > EARLIEST_QUARTER) return -1;
	if (quarter < CURRENT_QUARTER) return -1;

	if (quarter == CURRENT_QUARTER) {
		return ::Company::Get(company)->cur_economy.expenses;
	}
	return ::Company::Get(company)->old_economy[quarter - 1].expenses;
}

/* static */ SQInteger ScriptCompany::GetQuarterlyCargoDelivered(ScriptCompany::CompanyID company, SQInteger quarter)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return -1;
	if (quarter > EARLIEST_QUARTER) return -1;
	if (quarter < CURRENT_QUARTER) return -1;

	if (quarter == CURRENT_QUARTER) {
		return ::Company::Get(company)->cur_economy.delivered_cargo.GetSum<OverflowSafeInt32>();
	}
	return ::Company::Get(company)->old_economy[quarter - 1].delivered_cargo.GetSum<OverflowSafeInt32>();
}

/* static */ SQInteger ScriptCompany::GetQuarterlyPerformanceRating(ScriptCompany::CompanyID company, SQInteger quarter)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return -1;
	if (quarter > EARLIEST_QUARTER) return -1;
	if (quarter <= CURRENT_QUARTER) return -1;

	return ::Company::Get(company)->old_economy[quarter - 1].performance_history;
}

/* static */ Money ScriptCompany::GetQuarterlyCompanyValue(ScriptCompany::CompanyID company, SQInteger quarter)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return -1;
	if (quarter > EARLIEST_QUARTER) return -1;
	if (quarter < CURRENT_QUARTER) return -1;

	if (quarter == CURRENT_QUARTER) {
		return ::CalculateCompanyValue(::Company::Get(company));
	}
	return ::Company::Get(company)->old_economy[quarter - 1].company_value;
}


/* static */ Money ScriptCompany::GetBankBalance(ScriptCompany::CompanyID company)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return -1;

	return ::Company::Get(company)->money;
}

/* static */ Money ScriptCompany::GetLoanAmount()
{
	ScriptCompany::CompanyID company = ResolveCompanyID(COMPANY_SELF);
	if (company == COMPANY_INVALID) return -1;

	return ::Company::Get(company)->current_loan;
}

/* static */ Money ScriptCompany::GetMaxLoanAmount()
{
	return _economy.max_loan;
}

/* static */ Money ScriptCompany::GetLoanInterval()
{
	return LOAN_INTERVAL;
}

/* static */ bool ScriptCompany::SetLoanAmount(Money loan)
{
	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, loan >= 0);
	EnforcePrecondition(false, ((int64_t)loan % GetLoanInterval()) == 0);
	EnforcePrecondition(false, loan <= GetMaxLoanAmount());
	EnforcePrecondition(false, (loan - GetLoanAmount() + GetBankBalance(COMPANY_SELF)) >= 0);

	if (loan == GetLoanAmount()) return true;

	Money amount = abs(loan - GetLoanAmount());

	if (loan > GetLoanAmount()) {
		return ScriptObject::Command<CMD_INCREASE_LOAN>::Do(LoanCommand::Amount, amount);
	} else {
		return ScriptObject::Command<CMD_DECREASE_LOAN>::Do(LoanCommand::Amount, amount);
	}
}

/* static */ bool ScriptCompany::SetMinimumLoanAmount(Money loan)
{
	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, loan >= 0);

	Money over_interval = (int64_t)loan % GetLoanInterval();
	if (over_interval != 0) loan += GetLoanInterval() - over_interval;

	EnforcePrecondition(false, loan <= GetMaxLoanAmount());

	SetLoanAmount(loan);

	return GetLoanAmount() == loan;
}

/* static */ bool ScriptCompany::ChangeBankBalance(CompanyID company, Money delta, ExpensesType expenses_type, TileIndex tile)
{
	EnforceDeityMode(false);
	EnforcePrecondition(false, expenses_type < (ExpensesType)::EXPENSES_END);
	EnforcePrecondition(false, tile == INVALID_TILE || ::IsValidTile(tile));

	company = ResolveCompanyID(company);
	EnforcePrecondition(false, company != COMPANY_INVALID);

	/* Network commands only allow 0 to indicate invalid tiles, not INVALID_TILE */
	return ScriptObject::Command<CMD_CHANGE_BANK_BALANCE>::Do(tile == INVALID_TILE ? (TileIndex)0U : tile, delta, (::CompanyID)company, (::ExpensesType)expenses_type);
}

/* static */ bool ScriptCompany::BuildCompanyHQ(TileIndex tile)
{
	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, ::IsValidTile(tile));

	return ScriptObject::Command<CMD_BUILD_OBJECT>::Do(tile, OBJECT_HQ, 0);
}

/* static */ TileIndex ScriptCompany::GetCompanyHQ(CompanyID company)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return INVALID_TILE;

	TileIndex loc = ::Company::Get(company)->location_of_HQ;
	return (loc == 0) ? INVALID_TILE : loc;
}

/* static */ bool ScriptCompany::SetAutoRenewStatus(bool autorenew)
{
	EnforceCompanyModeValid(false);
	return ScriptObject::Command<CMD_CHANGE_COMPANY_SETTING>::Do("company.engine_renew", autorenew ? 1 : 0);
}

/* static */ bool ScriptCompany::GetAutoRenewStatus(CompanyID company)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return false;

	return ::Company::Get(company)->settings.engine_renew;
}

/* static */ bool ScriptCompany::SetAutoRenewMonths(SQInteger months)
{
	EnforceCompanyModeValid(false);
	months = Clamp<SQInteger>(months, INT16_MIN, INT16_MAX);

	return ScriptObject::Command<CMD_CHANGE_COMPANY_SETTING>::Do("company.engine_renew_months", months);
}

/* static */ SQInteger ScriptCompany::GetAutoRenewMonths(CompanyID company)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return 0;

	return ::Company::Get(company)->settings.engine_renew_months;
}

/* static */ bool ScriptCompany::SetAutoRenewMoney(Money money)
{
	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, money >= 0);
	EnforcePrecondition(false, (int64_t)money <= UINT32_MAX);
	return ScriptObject::Command<CMD_CHANGE_COMPANY_SETTING>::Do("company.engine_renew_money", money);
}

/* static */ Money ScriptCompany::GetAutoRenewMoney(CompanyID company)
{
	company = ResolveCompanyID(company);
	if (company == COMPANY_INVALID) return 0;

	return ::Company::Get(company)->settings.engine_renew_money;
}

/* static */ bool ScriptCompany::SetPrimaryLiveryColour(LiveryScheme scheme, Colours colour)
{
	EnforceCompanyModeValid(false);
	return ScriptObject::Command<CMD_SET_COMPANY_COLOUR>::Do((::LiveryScheme)scheme, true, (::Colours)colour);
}

/* static */ bool ScriptCompany::SetSecondaryLiveryColour(LiveryScheme scheme, Colours colour)
{
	EnforceCompanyModeValid(false);
	return ScriptObject::Command<CMD_SET_COMPANY_COLOUR>::Do((::LiveryScheme)scheme, false, (::Colours)colour);
}

/* static */ ScriptCompany::Colours ScriptCompany::GetPrimaryLiveryColour(ScriptCompany::LiveryScheme scheme)
{
	if ((::LiveryScheme)scheme < LS_BEGIN || (::LiveryScheme)scheme >= LS_END) return COLOUR_INVALID;

	const Company *c = ::Company::GetIfValid(_current_company);
	if (c == nullptr) return COLOUR_INVALID;

	return (ScriptCompany::Colours)c->livery[scheme].colour1;
}

/* static */ ScriptCompany::Colours ScriptCompany::GetSecondaryLiveryColour(ScriptCompany::LiveryScheme scheme)
{
	if ((::LiveryScheme)scheme < LS_BEGIN || (::LiveryScheme)scheme >= LS_END) return COLOUR_INVALID;

	const Company *c = ::Company::GetIfValid(_current_company);
	if (c == nullptr) return COLOUR_INVALID;

	return (ScriptCompany::Colours)c->livery[scheme].colour2;
}
