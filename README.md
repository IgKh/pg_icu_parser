# pg_icu_parser
PostgreSQL text search parser using ICU boundary analysis

## Overview

The first stage of full text search is parsing - breaking document and query texts into individual tokens (distinct words, numbers, etc). However there are languages where this task is not so trivial. The most prominent example is East Asian languages (such as Chinese, Japanese, Korean and more) - where words aren't typically separated by whitespace and punctuation. Another example is Hebrew; while words are separated by punctuation, some characters might be considered as punctuation marks or not depending on the context.

The default parser included with PostgreSQL's full text search subsystem provides rather unsatisfactory results in those cases. `pg_icu_parser` is an extension which provides a custom full text search parser implementation that use [ICU word boundary analysis](https://unicode-org.github.io/icu/userguide/boundaryanalysis/#word-boundary) routines to extract tokens out of source text. These implement the algorithms specified in Annex 29 of the Unicode standard, and can provide reasonable results across many languages.

## Requirements

* PostgreSQL 10+, compiled with ICU support

## Installation

Currently `pg_icu_parser` has to be built from source code. Make sure you have development support files (headers etc) for PostgreSQL available.

To build and install, run:
```
$ make install
```

This will build against and install into the PostgreSQL installation determined by the first instance of `pg_config` found in the current PATH. To target a specific installation (or one not in PATH):

```
$ make install PG_CONFIG=/path/to/pg_config
```

The extension is also available in [PGXN](https://pgxn.org/dist/pg_icu_parser/).

## Usage

To load the extension into a database, execute the following SQL command as a user with suitable permissions:

```sql
CREATE EXTENSION pg_icu_parser;
```

This will register the custom text parser in the current schema. To make use of it, a text search configuration must be created as described in the [PostgreSQL manual](https://www.postgresql.org/docs/current/textsearch-configuration.html). For example:

```sql
CREATE TEXT SEARCH CONFIGURATION sample (parser = icu_parser);
ALTER TEXT SEARCH CONFIGURATION sample ADD MAPPING FOR word WITH english_stem;
ALTER TEXT SEARCH CONFIGURATION sample ADD MAPPING FOR number, ideo, kana WITH simple;
```

The token types which can be emitted by the parser are `word`, `number`, `kana`, `ideo` and `blank` - these align with the word break tags supported by ICU. There are no restrictions as to which dictionaries can be used.

## GUC Parameters

`pg_icu_parser` exposes a single configuration parameter:

* `pg_icu_parser.locale` - string, optional. The ICU locale to use with the boundary analysis routines. If not set, defaults to `en`. Typically there is not need to set this parameter, as there are no word boundary detection rules which are sensitive to the locale.

## Comparing with the Default Parser

As described above, the main strength of `pg_icu_parser` versus the `default` parser already built into PostgreSQL are the better tokenization results across various languages. In addition, `pg_icu_parser` does not depend on the database's locale setting or the underlying operating system for determining what is a letter.

However, there are several trade-offs to be aware of which can impact decision whether `pg_icu_parser` is appropriate for a particular use case:

* The default parser can recognize a [wide variety](https://www.postgresql.org/docs/current/textsearch-parsers.html) of patterns as tokens, including URLs, e-mail addresses, various distinct kinds of numeric values, etc. `pg_icu_parser` on the other hand has significantly less token types (and the few that are supported are much more coarse), and can't detect any complex patterns.

* At the moment `pg_icu_parser` does **not** support the `ts_headline` function. Maybe in the future...

* `pg_icu_parser` is much slower than the `default` parser. However, if the server encoding of the database is UTF-8 a fast path is used, which reduces the overhead. See the benchmark section below.

* As a 3rd party extension, `pg_icu_parser` can only be used where full control of the PostgreSQL installation is available; that is, it probably can't be used with managed PostgreSQL solutions.

## Benchmark

The following non-scientific benchmark can give a sense of how much of a slow down one might expect from using `pg_icu_parser` instead of the default parser. The scenario tested is a query to count the number of non-blank tokens in each document in a corpus. The corpus used is the [HaAretz Corpus](https://mila.cs.technion.ac.il/eng/resources_corpora_haaretz.html), consisting of 27,139 short Hebrew language articles. Values are the average execution time in milliseconds of 10 rounds, as reported by `EXPLAIN ANALYZE`:

| Server encoding | `default`  | `pg_icu_parser` | Slowdown |
|-----------------|------------|-----------------|----------|
| UTF-8           | 2,566.9974 | 2,884.3696      | -12.3%   |
| ISO-8859-8      | 3,529.6487 | 5,059.3616      | -43.3%   |

Of course different environments and different corpora may have different results, YMMV.

## License

`pg_icu_parser` is licensed under the Mozilla Public License 2.0.
