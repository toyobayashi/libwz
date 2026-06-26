import { resolve } from 'node:path'
import HtmlWebpackPlugin from 'html-webpack-plugin'

export default {
  mode: 'production',
  target: 'web',
  entry: '../src/main.tsx',
  output: {
    clean: true,
    filename: 'bundle.js',
    library: {
      type: 'module'
    },
    module: true,
    path: resolve(import.meta.dirname, 'dist')
  },
  experiments: {
    outputModule: true
  },
  devServer: {
    client: {
      overlay: false
    },
    host: '127.0.0.1',
    hot: false,
    port: 5174,
    static: {
      directory: resolve(import.meta.dirname, 'dist')
    }
  },
  plugins: [
    new HtmlWebpackPlugin({
      scriptLoading: 'module',
      template: resolve(import.meta.dirname, 'index.html')
    })
  ],
  module: {
    rules: [
      {
        resourceQuery: /raw/,
        type: 'asset/source'
      },
      {
        exclude: /node_modules/,
        test: /\.tsx?$/,
        use: {
          loader: 'ts-loader',
          options: {
            compilerOptions: {
              noEmit: false
            },
            configFile: resolve(import.meta.dirname, '../tsconfig.json'),
            transpileOnly: false
          }
        }
      }
    ]
  },
  resolve: {
    alias: {
      libwz: resolve(import.meta.dirname, '../../../dist/index.js')
    },
    extensions: ['.tsx', '.ts', '.mjs', '.js']
  }
}
