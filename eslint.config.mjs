import stylistic from '@stylistic/eslint-plugin'
import love from 'eslint-config-love'

const sourceFiles = [
  '**/*.js',
  '**/*.mjs',
  '**/*.ts',
  '**/*.tsx'
]

export default [
  {
    ignores: [
      'build/**',
      'dist/**',
      'node_modules/**',
      'deps/**',
      'Harepacker-resurrected/**',
      'index.d.ts',
      'index.js',
      'js/*.js',
      'java/**/target/**',
      'tests/wasm/vite/dist/**',
      'tests/wasm/webpack/dist/**',
      '**/*.d.ts'
    ]
  },
  {
    ...love,
    files: sourceFiles,
    plugins: {
      ...love.plugins,
      '@stylistic': stylistic
    },
    languageOptions: {
      ...love.languageOptions,
      parserOptions: {
        ...love.languageOptions.parserOptions,
        project: './tsconfig.eslint.json',
        projectService: false,
        tsconfigRootDir: import.meta.dirname
      }
    },
    rules: {
      ...love.rules,
      '@typescript-eslint/class-methods-use-this': 'off',
      '@typescript-eslint/explicit-function-return-type': 'off',
      '@typescript-eslint/init-declarations': 'off',
      '@typescript-eslint/max-params': 'off',
      '@typescript-eslint/method-signature-style': 'off',
      '@typescript-eslint/naming-convention': 'off',
      '@typescript-eslint/consistent-type-assertions': 'off',
      '@typescript-eslint/no-confusing-void-expression': 'off',
      '@typescript-eslint/no-empty-function': 'off',
      '@typescript-eslint/no-empty-object-type': 'off',
      '@typescript-eslint/no-extraneous-class': 'off',
      '@typescript-eslint/no-magic-numbers': 'off',
      '@typescript-eslint/no-redundant-type-constituents': 'off',
      '@typescript-eslint/no-unnecessary-condition': 'off',
      '@typescript-eslint/no-unsafe-declaration-merging': 'off',
      '@typescript-eslint/no-unsafe-function-type': 'off',
      '@typescript-eslint/no-unsafe-argument': 'off',
      '@typescript-eslint/no-unsafe-assignment': 'off',
      '@typescript-eslint/no-unsafe-call': 'off',
      '@typescript-eslint/no-unsafe-member-access': 'off',
      '@typescript-eslint/no-unsafe-return': 'off',
      '@typescript-eslint/no-unsafe-type-assertion': 'off',
      '@typescript-eslint/prefer-destructuring': 'off',
      '@typescript-eslint/prefer-nullish-coalescing': 'off',
      '@typescript-eslint/require-await': 'off',
      '@typescript-eslint/strict-boolean-expressions': 'off',
      '@typescript-eslint/unified-signatures': 'off',
      'complexity': 'off',
      'max-lines': 'off',
      'require-unicode-regexp': 'off',
      '@stylistic/array-bracket-spacing': ['error', 'never'],
      '@stylistic/arrow-parens': ['error', 'always'],
      '@stylistic/brace-style': ['error', '1tbs', { allowSingleLine: true }],
      '@stylistic/comma-dangle': ['error', 'never'],
      '@stylistic/comma-spacing': ['error', { before: false, after: true }],
      '@stylistic/eol-last': ['error', 'always'],
      '@stylistic/indent': ['error', 2, { SwitchCase: 1 }],
      '@stylistic/key-spacing': ['error', { beforeColon: false, afterColon: true }],
      '@stylistic/keyword-spacing': ['error', { before: true, after: true }],
      '@stylistic/member-delimiter-style': ['error', {
        multiline: {
          delimiter: 'semi',
          requireLast: true
        },
        singleline: {
          delimiter: 'semi',
          requireLast: false
        }
      }],
      '@stylistic/no-multiple-empty-lines': ['error', { max: 1, maxBOF: 0, maxEOF: 0 }],
      '@stylistic/no-trailing-spaces': 'error',
      '@stylistic/object-curly-spacing': ['error', 'always'],
      '@stylistic/operator-linebreak': ['error', 'before'],
      '@stylistic/quotes': ['error', 'single', { avoidEscape: true }],
      '@stylistic/semi': ['error', 'never'],
      '@stylistic/space-before-blocks': ['error', 'always'],
      '@stylistic/space-before-function-paren': ['error', 'always'],
      '@stylistic/space-in-parens': ['error', 'never'],
      '@stylistic/space-infix-ops': 'error',
      '@stylistic/type-annotation-spacing': 'error'
    }
  },
  {
    files: [
      'eslint.config.mjs',
      'scripts/**/*.mjs',
      'tests/**/*.js',
      'tests/**/*.mjs'
    ],
    rules: {
      '@typescript-eslint/no-implicit-any-catch': 'off',
      '@typescript-eslint/no-require-imports': 'off',
      'import/no-extraneous-dependencies': 'off',
      'n/no-unsupported-features/es-syntax': 'off'
    }
  },
  {
    files: ['tests/wasm/src/**/*.ts', 'tests/wasm/src/**/*.tsx'],
    rules: {
      'import/no-extraneous-dependencies': 'off',
      'promise/avoid-new': 'off'
    }
  }
]
