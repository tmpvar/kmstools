require('fs').createReadStream('/dev/input/event13').on('data', function(d) {
  console.log(d.length,
    'type', d.readUInt16LE(16),
    'code', d.readUInt16LE(18),
    'value', d.readInt32LE(20)
  );
});
