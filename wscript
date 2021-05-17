# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('wildfire', ['applications'])
    module.source = [
        'model/wildfire-server.cc',
        'model/wildfire-client.cc',
        'model/wildfire-message.cc',
        'helper/wildfire-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('wildfire')
    module_test.source = [
        'test/wildfire-test-suite.cc',
        ]
    # Tests encapsulating example programs should be listed here
    if (bld.env['ENABLE_EXAMPLES']):
        module_test.source.extend([
        #    'test/wildfire-examples-test-suite.cc',
             ])

    headers = bld(features='ns3header')
    headers.module = 'wildfire'
    headers.source = [
        'model/wildfire-server.h',
        'model/wildfire-client.h',
        'model/wildfire-message.h',
        'helper/wildfire-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

