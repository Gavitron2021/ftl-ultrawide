"""Optional, reversible NVIDIA DXGI presentation for this installation only.

Uses NVIDIA's public DRS API. Never edits global or pre-existing user profiles.
Reference: https://github.com/NVIDIA/nvapi (nvapi.h, NvApiDriverSettings.h).
"""
from pathlib import Path
import ctypes as C
import hashlib
import os

SETTING_ID = 0x20D690F8  # OGL_CPL_PREFER_DXPRESENT_ID
DXGI = 1
MARKER = 'ultrawide-runtime.cfg'


def require(condition, message):
    if not condition:
        raise ValueError('NVIDIA presentation: ' + message)


class Binary(C.Structure):
    _fields_ = [('length', C.c_uint32), ('data', C.c_ubyte * 4096)]


class Value(C.Union):
    _pack_ = 4
    _fields_ = [('integer', C.c_uint32), ('binary', Binary),
                ('text', C.c_wchar * 2048), ('qword', C.c_uint64)]


class Setting(C.Structure):
    _pack_ = 4
    _fields_ = [('version', C.c_uint32), ('name', C.c_wchar * 2048),
                ('id', C.c_uint32), ('type', C.c_uint32),
                ('location', C.c_uint32), ('predefined', C.c_uint32),
                ('default_valid', C.c_uint32), ('default', Value), ('current', Value)]


class Application(C.Structure):
    _fields_ = [('version', C.c_uint32), ('predefined', C.c_uint32),
                ('name', C.c_wchar * 2048), ('friendly', C.c_wchar * 2048),
                ('launcher', C.c_wchar * 2048), ('marker', C.c_wchar * 2048),
                ('flags', C.c_uint32), ('command_line', C.c_wchar * 2048)]


class Profile(C.Structure):
    _fields_ = [('version', C.c_uint32), ('name', C.c_wchar * 2048),
                ('gpu_support', C.c_uint32), ('predefined', C.c_uint32),
                ('apps', C.c_uint32), ('settings', C.c_uint32)]


def versioned(kind, version=1):
    value = kind()
    value.version = C.sizeof(value) | (version << 16)
    return value


class Drs:
    def __init__(self):
        require(os.name == 'nt', 'the driver option requires Windows')
        library = 'nvapi64.dll' if C.sizeof(C.c_void_p) == 8 else 'nvapi.dll'
        path = Path(os.environ['WINDIR']) / 'System32' / library
        require(path.is_file(), 'NVIDIA driver API is unavailable; omit --nvidia-dxgi')
        self.library = C.CDLL(str(path))
        self.query = self.library.nvapi_QueryInterface
        self.query.argtypes = [C.c_uint32]
        self.query.restype = C.c_void_p
        self.session = C.c_void_p()
        self.check(self.api(0x0150E828)())  # Initialize
        self.check(self.api(0x0694D52E, C.POINTER(C.c_void_p))(C.byref(self.session)))
        try:
            self.check(self.api(0x375DBD6B, C.c_void_p)(self.session))
        except Exception:
            self.close()
            raise

    def api(self, ident, *types):
        address = self.query(ident)
        require(bool(address), f'driver API 0x{ident:x} is unavailable')
        return C.CFUNCTYPE(C.c_int, *types)(address)

    @staticmethod
    def check(status):
        require(status == 0, f'driver returned {status}')

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()

    def close(self):
        if self.session.value:
            self.api(0xDAD9CFF8, C.c_void_p)(self.session)
            self.session = C.c_void_p()

    def _profile(self, name):
        handle = C.c_void_p()
        status = self.api(0x7E4A9A0B, C.c_void_p, C.c_wchar_p, C.POINTER(C.c_void_p))(
            self.session, name, C.byref(handle))
        if status == -163:  # PROFILE_NOT_FOUND
            return None
        self.check(status)
        return handle

    def _info(self, handle):
        value = versioned(Profile)
        self.check(self.api(0x61CD6FD6, C.c_void_p, C.c_void_p, C.POINTER(Profile))(
            self.session, handle, C.byref(value)))
        return {'name': value.name, 'predefined': bool(value.predefined),
                'apps': value.apps, 'settings': value.settings}

    def lookup_application(self, path):
        handle = C.c_void_p()
        app = versioned(Application, 4)
        status = self.api(0xEEE566B2, C.c_void_p, C.c_wchar_p,
                          C.POINTER(C.c_void_p), C.POINTER(Application))(
            self.session, path, C.byref(handle), C.byref(app))
        if status == -166:  # EXECUTABLE_NOT_FOUND
            return None
        self.check(status)
        return self._info(handle)

    def inspect(self, name):
        handle = self._profile(name)
        if handle is None:
            return None
        info = self._info(handle)
        if info['apps'] == 1:
            count = C.c_uint32(1)
            app = versioned(Application, 4)
            self.check(self.api(0x7FA2173A, C.c_void_p, C.c_void_p, C.c_uint32,
                                C.POINTER(C.c_uint32), C.POINTER(Application))(
                self.session, handle, 0, C.byref(count), C.byref(app)))
            require(count.value == 1, 'profile application count changed')
            info.update(application=app.name, marker=app.marker)
        if info['settings'] == 1:
            setting = versioned(Setting)
            status = self.api(0x73BF8338, C.c_void_p, C.c_void_p, C.c_uint32,
                              C.POINTER(Setting))(
                self.session, handle, SETTING_ID, C.byref(setting))
            self.check(status)
            info.update(setting=setting.current.integer,
                        setting_location=setting.location, setting_type=setting.type)
        return info

    def backup(self, path):
        require(not path.exists(), 'refusing to overwrite a driver settings backup')
        self.check(self.api(0x2BE25DF8, C.c_void_p, C.c_wchar_p)(self.session, str(path)))
        require(path.is_file() and path.stat().st_size > 0, 'driver settings backup is empty or missing')

    def create(self, plan):
        handle = C.c_void_p()
        profile = versioned(Profile)
        profile.name = plan['profileName']
        self.check(self.api(0xCC176068, C.c_void_p, C.POINTER(Profile), C.POINTER(C.c_void_p))(
            self.session, C.byref(profile), C.byref(handle)))
        app = versioned(Application, 4)
        app.name = plan['application']
        app.friendly = 'FTL Ultrawide'
        app.marker = MARKER
        self.check(self.api(0x4347A9DE, C.c_void_p, C.c_void_p, C.POINTER(Application))(
            self.session, handle, C.byref(app)))
        setting = versioned(Setting)
        setting.id = SETTING_ID
        setting.current.integer = DXGI
        self.check(self.api(0x577DD202, C.c_void_p, C.c_void_p, C.POINTER(Setting))(
            self.session, handle, C.byref(setting)))

    def delete(self, name):
        handle = self._profile(name)
        require(handle is not None, 'owned profile disappeared')
        self.check(self.api(0x17093206, C.c_void_p, C.c_void_p)(self.session, handle))

    def save(self):
        self.check(self.api(0xFCBC7E14, C.c_void_p)(self.session))
        self.check(self.api(0x375DBD6B, C.c_void_p)(self.session))


def validate_plan(plan):
    application = str(Path(plan['application']).resolve())
    require(Path(application).name.lower() == 'ftlgame-1.6.9.exe', 'unexpected executable name')
    key = hashlib.sha256(application.casefold().encode('utf-8')).hexdigest()[:16]
    require(plan['profileName'] == 'FTL Ultrawide DXGI ' + key, 'unexpected profile identity')
    require(plan['settingId'] == SETTING_ID and plan['settingValue'] == DXGI
            and plan['marker'] == MARKER and type(plan['changed']) is bool,
            'invalid profile transaction')


def check_owned(info, plan):
    require(info is not None and info['name'] == plan['profileName']
            and not info['predefined'] and info['apps'] == 1 and info['settings'] == 1,
            'profile was customized; inspect it before restoring')
    # DRS canonicalizes application names to lowercase forward-slash paths.
    require(str(Path(info.get('application', '')).resolve()).casefold()
            == str(Path(plan['application']).resolve()).casefold()
            and info.get('marker') == MARKER and info.get('setting') == DXGI
            and info.get('setting_location') == 0 and info.get('setting_type') == 0,
            'profile contents changed; inspect it before restoring')


def plan(game):
    application = str((Path(game).resolve() / 'FTLGame-1.6.9.exe'))
    key = hashlib.sha256(application.casefold().encode('utf-8')).hexdigest()[:16]
    result = {'application': application, 'profileName': 'FTL Ultrawide DXGI ' + key,
              'marker': MARKER, 'settingId': SETTING_ID, 'settingValue': DXGI, 'changed': True}
    with Drs() as driver:
        existing = driver.lookup_application(application)
        owned = driver.inspect(result['profileName'])
        if existing is not None:
            require(existing['name'] == result['profileName'],
                    'an existing profile controls this game; configure presentation there or omit --nvidia-dxgi')
            check_owned(owned, result)
            result['changed'] = False
        else:
            require(owned is None, 'profile name collision; no driver settings were changed')
    return result


def apply(plan, backup):
    validate_plan(plan)
    require(Path(plan['application']).is_file()
            and (Path(plan['application']).parent / MARKER).is_file(), 'installed files are missing')
    with Drs() as driver:
        if not plan['changed']:
            check_owned(driver.inspect(plan['profileName']), plan)
            return
        require(driver.lookup_application(plan['application']) is None
                and driver.inspect(plan['profileName']) is None,
                'profile changed after review; no driver settings were changed')
        driver.backup(Path(backup) / 'nvidia-before.dat')
        driver.create(plan)
        driver.save()
        check_owned(driver.inspect(plan['profileName']), plan)


def check_restore(plan):
    validate_plan(plan)
    if plan['changed']:
        with Drs() as driver:
            info = driver.inspect(plan['profileName'])
            if info is not None:
                check_owned(info, plan)


def restore(plan):
    validate_plan(plan)
    if plan['changed']:
        with Drs() as driver:
            info = driver.inspect(plan['profileName'])
            if info is None:
                return
            check_owned(info, plan)
            driver.delete(plan['profileName'])
            driver.save()
            require(driver.inspect(plan['profileName']) is None, 'profile removal verification failed')
