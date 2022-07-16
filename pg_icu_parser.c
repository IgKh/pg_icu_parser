/*
 * pg_icu_parser.c
 *
 * Copyright (c) 2022 Igor Khanin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include <postgres.h>

#include <fmgr.h>
#include <tsearch/ts_public.h>
#include <utils/elog.h>
#include <utils/guc.h>
#include <utils/pg_locale.h>

#ifndef USE_ICU
#error Must build against PostgreSQL compiled with ICU support
#endif //USE_ICU

#include <unicode/ubrk.h>

//
// Output Token Categories
//
#define WORD_TOKEN     1
#define NUMBER_TOKEN   2
#define BLANK_TOKEN    3
#define KANA_TOKEN     4
#define IDEO_TOKEN     5

#define NUM_TOKENS     5

PG_MODULE_MAGIC;

void _PG_init(void);

PG_FUNCTION_INFO_V1(icuparser_lextype);
PG_FUNCTION_INFO_V1(icuparser_start);
PG_FUNCTION_INFO_V1(icuparser_nexttoken);
PG_FUNCTION_INFO_V1(icuparser_end);
PG_FUNCTION_INFO_V1(icuparser_headline);

typedef struct
{
    UChar*          utext;
    UBreakIterator* iter;
    int32_t         token_start_pos;
    int32_t         token_end_pos;
    char*           curr_token;
}
ParserState;

static char *locale_guc = "";

void
_PG_init(void)
{
    DefineCustomStringVariable(
        "pg_icu_parser.locale",                        // name
        "ICU locale name to use in boundry analysis.", // short_desc
        NULL,                                          // long_desc
        &locale_guc,                                   // valueAddr
        "",                                            // bootValue
        PGC_USERSET,                                   // context
        0,                                             // flags
        NULL, NULL, NULL);                             // check_hook, assign_hook, show_hook

    EmitWarningsOnPlaceholders("pg_icu_parser");
}

Datum
icuparser_lextype(PG_FUNCTION_ARGS)
{
    LexDescr* descr = (LexDescr*)palloc(sizeof(LexDescr) * (NUM_TOKENS + 1));

    descr[0].lexid = WORD_TOKEN;
    descr[0].alias = pstrdup("word");
    descr[0].descr = pstrdup("Word, containing normal letters");

    descr[1].lexid = NUMBER_TOKEN;
    descr[1].alias = pstrdup("number");
    descr[1].descr = pstrdup("Numeric value");

    descr[2].lexid = BLANK_TOKEN;
    descr[2].alias = pstrdup("blank");
    descr[2].descr = pstrdup("Space and punctuation symbols");

    descr[3].lexid = KANA_TOKEN;
    descr[3].alias = pstrdup("kana");
    descr[3].descr = pstrdup("Word, composed of Japanese Kana characters");

    descr[4].lexid = IDEO_TOKEN;
    descr[4].alias = pstrdup("ideo");
    descr[4].descr = pstrdup("Ideographic characters");

    descr[5].lexid = 0;

    PG_RETURN_POINTER(descr);
}

Datum
icuparser_start(PG_FUNCTION_ARGS)
{
    char* text     = (char*)PG_GETARG_POINTER(0);
    int   text_len = (int)PG_GETARG_INT32(1);

    ParserState* state;
    char*        locale;
    int32_t      utext_len;
    UErrorCode   rc;

    state = (ParserState*)palloc0(sizeof(ParserState));

    locale = locale_guc;
    if (*locale == 0) {
        locale = "en";
    }

    utext_len = icu_to_uchar(&state->utext, text, text_len);

    rc = U_ZERO_ERROR;
    state->iter = ubrk_open(UBRK_WORD, locale, state->utext, utext_len, &rc);
    if (U_FAILURE(rc)) {
        ereport(ERROR,
				(errmsg("could not create ICU word break iterator for locale %s: %s",
						locale, u_errorName(rc))));
    }

    state->token_start_pos = ubrk_first(state->iter);
    state->token_end_pos = ubrk_next(state->iter);
    state->curr_token = NULL;

    PG_RETURN_POINTER(state);
}

static int
convert_token_type(int32_t icu_type)
{
    if (icu_type >= UBRK_WORD_LETTER && icu_type < UBRK_WORD_LETTER_LIMIT) {
        return WORD_TOKEN;
    }
    if (icu_type >= UBRK_WORD_NUMBER && icu_type < UBRK_WORD_NUMBER_LIMIT) {
        return NUMBER_TOKEN;
    }
    if (icu_type >= UBRK_WORD_KANA && icu_type < UBRK_WORD_KANA_LIMIT) {
        return KANA_TOKEN;
    }
    if (icu_type >= UBRK_WORD_IDEO && icu_type < UBRK_WORD_IDEO_LIMIT) {
        return IDEO_TOKEN;
    }
    return BLANK_TOKEN;
}

Datum
icuparser_nexttoken(PG_FUNCTION_ARGS)
{
    ParserState* state         = (ParserState*)PG_GETARG_POINTER(0);
    char**       out_token     = (char**)PG_GETARG_POINTER(1);
    int*         out_token_len = (int*)PG_GETARG_POINTER(2);

    int32_t token_type;

    if (state->token_end_pos == UBRK_DONE) {
        PG_RETURN_INT32(0);
    }

    if (state->curr_token != NULL) {
        pfree(state->curr_token);
        state->curr_token = NULL;
    }

    *out_token_len = icu_from_uchar(
        &state->curr_token,
        state->utext + state->token_start_pos,
        state->token_end_pos - state->token_start_pos);

    *out_token = state->curr_token;
    token_type = ubrk_getRuleStatus(state->iter);

    state->token_start_pos = state->token_end_pos;
    state->token_end_pos = ubrk_next(state->iter);

    PG_RETURN_INT32(convert_token_type(token_type));
}

Datum
icuparser_end(PG_FUNCTION_ARGS)
{
    ParserState* state = (ParserState*)PG_GETARG_POINTER(0);

    if (state->curr_token != NULL) {
        pfree(state->curr_token);
    }

    ubrk_close(state->iter);
    pfree(state->utext);
    pfree(state);

    PG_RETURN_VOID();
}

Datum
icuparser_headline(PG_FUNCTION_ARGS)
{
    // Placeholder to make it easier to implement in the future
    ereport(ERROR,
		    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("icu_parser does not support headline creation")));
}
