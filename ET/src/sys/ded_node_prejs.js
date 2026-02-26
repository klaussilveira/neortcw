Module['masterServerUrl'] = process.env.MASTER_URL || 'wss://et-master.klaus-silveira.workers.dev';

Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function() {
    FS.mkdir('/base');
    FS.mount(NODEFS, { root: '.' }, '/base');
    FS.chdir('/base');
});
