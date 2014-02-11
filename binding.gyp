{
  'targets': [
    {
      'target_name': 'purple',
      'sources': [ 'binding.cc' ],
      'include_dirs' : [
	   '/usr/include/libpurple',
	   '/usr/include/glib-2.0',
	   '/usr/lib/x86_64-linux-gnu/glib-2.0/include',
       "<!(node -e \"require('nan')\")"
      ],
      'conditions': [
        ['OS=="linux"',
		{
          'libraries': [
           '-lpurple',
			'-Wl,-rpath ./'
          ],
		  
        }],
      ]
    }
  ]
}
