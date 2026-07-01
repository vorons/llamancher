import { derived, writable, type Readable } from 'svelte/store';
import en from './en.json';
import ru from './ru.json';

export type Locale = 'en' | 'ru';

const dict: Record<Locale, Record<string, string>> = { en, ru };

export function detectSystemLocale(): Locale {
  const lang = (navigator.language || '').split('-')[0];
  return lang === 'ru' ? 'ru' : 'en';
}

/** Resolved current locale. Set this or use applyLocale(). */
export const locale = writable<Locale>(detectSystemLocale());

/** Reactive translate function. Usage: {$t('key')} */
export const t: Readable<(key: string, params?: Record<string, string | number>) => string> = derived(
  locale,
  ($locale) => {
    const msgs = dict[$locale] ?? dict.en;
    return (key: string, params?: Record<string, string | number>): string => {
      let val = msgs[key];
      if (val === undefined) val = key;
      if (params) {
        for (const [k, v] of Object.entries(params)) {
          val = val.replace(`{${k}}`, String(v));
        }
      }
      return val;
    };
  },
);

/** Resolve and apply a locale preference (auto = follow system). */
export function applyLocale(setting: Locale | 'auto'): void {
  locale.set(setting === 'auto' ? detectSystemLocale() : setting);
}
