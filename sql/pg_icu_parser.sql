CREATE EXTENSION pg_icu_parser;

SELECT * FROM ts_parse('icu_parser', NULL);
SELECT * FROM ts_parse('icu_parser', '');

SELECT * FROM ts_parse('icu_parser', 'A regular "English" sentence with less than 100 words');

SELECT * FROM ts_parse('icu_parser', 'משפט פשוט בעברית');
SELECT * FROM ts_parse('icu_parser', 'אתמול נסענו לנתב"ג עם ג''ורג'' הקטן');

SELECT * from ts_parse('icu_parser', '今日もＪＲ東日本お後リヨくださいましてありがとゴザます。');
