MODULES = pg_icu_parser

EXTENSION = pg_icu_parser
DATA = pg_icu_parser--1.0.sql

REGRESS = pg_icu_parser

PG_CONFIG = pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
