import assert from 'node:assert/strict';
import test from 'node:test';
import { Character, emotions } from '../character/emotions.mjs';

test('all 30 emotions produce a bounded pose, tone and touch response', () => {
  assert.equal(new Set(emotions).size, 30);
  for (const emotion of emotions) {
    const character = new Character();
    assert.ok(character.select(emotion, 0).frequency > 0);
    const pose = character.frame(500, { gazeX: 3, gazeY: -3 });
    assert.equal(pose.emotion, emotion);
    assert.ok(pose.eyes > 0 && pose.eyes <= 1.2);
    assert.ok(pose.mouth >= 0 && pose.mouth <= 1);
    assert.equal(pose.gazeX, 1);
    assert.equal(pose.gazeY, -1);
    assert.ok(character.touch(501).durationMs > 0);
  }
});

test('switching emotions midway preserves the current eye pose', () => {
  const character = new Character();
  character.select('sleepy', 10);
  const before = character.frame(110).eyes;
  character.select('happy', 110);
  assert.equal(character.frame(110).eyes, before);
  assert.equal(character.frame(330).eyes, 1);
});

test('speaking responds to audio activity and unknown states are rejected', () => {
  const character = new Character();
  character.select('speaking', 0);
  assert.ok(character.frame(300, { audioLevel: 0.7 }).mouth > character.frame(300).mouth);
  assert.throws(() => character.select('constructor', 500), RangeError);
});
