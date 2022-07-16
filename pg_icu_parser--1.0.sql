-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_icu_parser" to load this file. \quit

CREATE OR REPLACE FUNCTION icuparser_start(internal, integer)
    RETURNS internal
    AS 'MODULE_PATHNAME', 'icuparser_start'
    LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION icuparser_nexttoken(internal, internal, internal)
    RETURNS internal
    AS 'MODULE_PATHNAME', 'icuparser_nexttoken'
    LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION icuparser_end(internal)
    RETURNS void
    AS 'MODULE_PATHNAME', 'icuparser_end'
    LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION icuparser_lextype(internal)
    RETURNS internal
    AS 'MODULE_PATHNAME', 'icuparser_lextype'
    LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION icuparser_headline(internal, internal, tsquery)
    RETURNS internal
    AS 'MODULE_PATHNAME', 'icuparser_headline'
    LANGUAGE C STRICT IMMUTABLE;

CREATE TEXT SEARCH PARSER icu_parser (
    START = icuparser_start,
    GETTOKEN = icuparser_nexttoken,
    END = icuparser_end,
    LEXTYPES = icuparser_lextype,
    HEADLINE = icuparser_headline
);
COMMENT ON TEXT SEARCH PARSER icu_parser IS 'ICU word boundry parser';
