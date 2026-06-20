"""STPyV8 stub for ARM — allows pypkjs to install; JS execution is unavailable."""


class STPyV8NotAvailableError(Exception):
    pass


def _raise_not_available():
    raise STPyV8NotAvailableError(
        "STPyV8 is not available on ARM. Emulator PKJS (settings/weather) will not run."
    )


class JSContext:
    def __init__(self, global_obj=None, extensions=None):
        _raise_not_available()

    def __enter__(self):
        _raise_not_available()

    def __exit__(self, exc_type, exc_val, exc_tb):
        _raise_not_available()

    def eval(self, code, filename=None):
        _raise_not_available()


class JSError(Exception):
    pass


class JSSyntaxError(JSError):
    def hint(self, src):
        return str(self)


class JSExtension:
    def __init__(self, name, code):
        self.name = name
        self.code = code


class JSEngine:
    version = "Not available on ARM"


class JSClass(type):
    def __new__(mcs, name_or_cls, bases=None, namespace=None, **kwargs):
        _raise_not_available()

    def __call__(cls, *args, **kwargs):
        _raise_not_available()


def evaljs(code):
    _raise_not_available()


STPyV8 = type('STPyV8', (), {
    'JSContext': JSContext,
    'JSError': JSError,
    'JSSyntaxError': JSSyntaxError,
    'JSExtension': JSExtension,
    'JSEngine': JSEngine,
    'JSClass': JSClass,
    'evaljs': staticmethod(evaljs),
})()

__all__ = [
    'STPyV8', 'JSContext', 'JSError', 'JSSyntaxError', 'JSExtension',
    'JSEngine', 'JSClass', 'evaljs', 'STPyV8NotAvailableError',
]
