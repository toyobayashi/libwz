"use strict";

import type { NativeBinding } from "./src/native-binding";
import { createWzApiFromBinding } from "./src/node-wrapper";
export type { DetectedMapleVersion } from "./src/native-binding";
declare const __dirname: string;
declare function require(id: string): unknown;

interface NodePathModule {
  join(...parts: string[]): string;
}

function loadNative(): NativeBinding {
  const path = require("node:path") as NodePathModule;
  const candidates = [
    path.join(__dirname, "build", "Release", "wz.node"),
    path.join(__dirname, "build", "Release", "libwz.node"),
    path.join(__dirname, "build", "Debug", "wz.node"),
    path.join(__dirname, "build", "Debug", "libwz.node"),
  ];
  for (const candidate of candidates) {
    try {
      return require(candidate) as NativeBinding;
    } catch (err: unknown) {
      const code = (err as { code?: string }).code;
      if (code !== "MODULE_NOT_FOUND") throw err;
    }
  }
  throw new Error("libwz native addon is not built. Run `npm run build`.");
}

const native = loadNative();

export const MapleVersion = Object.freeze({
  GMS: 0,
  EMS: 1,
  BMS: 2,
  CLASSIC: 3,
  GENERATE: 4,
  GETFROMZLZ: 5,
  CUSTOM: 6,
  UNKNOWN: 99,
} as const);
export type MapleVersionValue = (typeof MapleVersion)[keyof typeof MapleVersion];

export const ParseStatus = Object.freeze({
  PATH_IS_NULL: -1,
  ERROR_GAME_VER_HASH: -2,
  FAILED_UNKNOWN: 0,
  SUCCESS: 1,
} as const);
export type ParseStatusValue = (typeof ParseStatus)[keyof typeof ParseStatus];

export const PropertyType = Object.freeze({
  NULL: 0,
  SHORT: 1,
  INT: 2,
  LONG: 3,
  FLOAT: 4,
  DOUBLE: 5,
  STRING: 6,
  SUB: 7,
  CANVAS: 8,
  VECTOR: 9,
  CONVEX: 10,
  SOUND: 11,
  RAW: 12,
  UOL: 13,
  LUA: 14,
  PNG: 15,
} as const);
export type PropertyTypeValue = (typeof PropertyType)[keyof typeof PropertyType];

export const ObjectType = Object.freeze({
  FILE: 0,
  IMAGE: 1,
  DIRECTORY: 2,
  PROPERTY: 3,
  LIST: 4,
} as const);
export type ObjectTypeValue = (typeof ObjectType)[keyof typeof ObjectType];

export const BinaryType = Object.freeze({
  RAW: 0,
  MP3: 1,
  WAV: 2,
} as const);
export type BinaryTypeValue = (typeof BinaryType)[keyof typeof BinaryType];


const api = createWzApiFromBinding(native, {
  blobInput: false,
  pathInput: true,
  saveToDisk: true,
});

export const WzObject = api.WzObject;
export type WzObject = InstanceType<typeof WzObject>;
export const WzFile = api.WzFile;
export type WzFile = InstanceType<typeof WzFile>;
export const WzDirectory = api.WzDirectory;
export type WzDirectory = InstanceType<typeof WzDirectory>;
export const WzImage = api.WzImage;
export type WzImage = InstanceType<typeof WzImage>;
export const WzPropertyCollection = api.WzPropertyCollection;
export type WzPropertyCollection = InstanceType<typeof WzPropertyCollection>;
export const WzImageProperty = api.WzImageProperty;
export type WzImageProperty = InstanceType<typeof WzImageProperty>;
export const WzNullProperty = api.WzNullProperty;
export type WzNullProperty = InstanceType<typeof WzNullProperty>;
export const WzShortProperty = api.WzShortProperty;
export type WzShortProperty = InstanceType<typeof WzShortProperty>;
export const WzIntProperty = api.WzIntProperty;
export type WzIntProperty = InstanceType<typeof WzIntProperty>;
export const WzLongProperty = api.WzLongProperty;
export type WzLongProperty = InstanceType<typeof WzLongProperty>;
export const WzFloatProperty = api.WzFloatProperty;
export type WzFloatProperty = InstanceType<typeof WzFloatProperty>;
export const WzDoubleProperty = api.WzDoubleProperty;
export type WzDoubleProperty = InstanceType<typeof WzDoubleProperty>;
export const WzStringProperty = api.WzStringProperty;
export type WzStringProperty = InstanceType<typeof WzStringProperty>;
export const WzSubProperty = api.WzSubProperty;
export type WzSubProperty = InstanceType<typeof WzSubProperty>;
export const WzCanvasProperty = api.WzCanvasProperty;
export type WzCanvasProperty = InstanceType<typeof WzCanvasProperty>;
export const WzPngProperty = api.WzPngProperty;
export type WzPngProperty = InstanceType<typeof WzPngProperty>;
export const WzVectorProperty = api.WzVectorProperty;
export type WzVectorProperty = InstanceType<typeof WzVectorProperty>;
export const WzConvexProperty = api.WzConvexProperty;
export type WzConvexProperty = InstanceType<typeof WzConvexProperty>;
export const WzBinaryProperty = api.WzBinaryProperty;
export type WzBinaryProperty = InstanceType<typeof WzBinaryProperty>;
export const WzRawDataProperty = api.WzRawDataProperty;
export type WzRawDataProperty = InstanceType<typeof WzRawDataProperty>;
export const WzVideoProperty = api.WzVideoProperty;
export type WzVideoProperty = InstanceType<typeof WzVideoProperty>;
export const WzUOLProperty = api.WzUOLProperty;
export type WzUOLProperty = InstanceType<typeof WzUOLProperty>;
export const WzLuaProperty = api.WzLuaProperty;
export type WzLuaProperty = InstanceType<typeof WzLuaProperty>;
export const WzProperty = api.WzProperty;
export type WzProperty = InstanceType<typeof WzProperty>;
export const WzTool = api.WzTool;
