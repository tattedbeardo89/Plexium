#!/usr/bin/env python3
"""Report Plexium-identity provider compatibility; never claim playback acceptance.

Uses a new anonymous session, not a personal account or third-party token. Does
not request a stream, spoof a region, advertise DRM, follow redirects or log tokens.
"""
import json
import urllib.error
import urllib.request
import uuid
from pathlib import Path

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None

opener = urllib.request.build_opener(NoRedirect)
headers = {'Accept': 'application/json', 'User-Agent': 'Plexium/0.2.0',
           'X-Plex-Product': 'Plexium', 'X-Plex-Version': '0.2.0',
           'X-Plex-Platform': 'PlayStation 5', 'X-Plex-Device': 'PS5',
           'X-Plex-Client-Identifier': str(uuid.uuid4())}

def request(url, method='GET', token=None):
    selected = dict(headers)
    if token:
        selected['X-Plex-Token'] = token
    req = urllib.request.Request(url, data=b'' if method == 'POST' else None,
                                 headers=selected, method=method)
    try:
        with opener.open(req, timeout=15) as response:
            body = response.read(8 * 1024 * 1024 + 1)
            if len(body) > 8 * 1024 * 1024:
                return response.status, {}
            try:
                return response.status, json.loads(body)
            except (ValueError, UnicodeError):
                return response.status, {}
    except urllib.error.HTTPError as error:
        return error.code, {}
    except (urllib.error.URLError, TimeoutError, OSError):
        return 0, {}

status, session = request('https://clients.plex.tv/api/v2/users/anonymous', 'POST')
token = session.get('authToken') or session.get('authenticationToken') if isinstance(session, dict) else None
if not isinstance(token, str):
    token = None
provider_status, provider = request('https://epg.provider.plex.tv/lineups/plex/channels?includeMetadata=1', token=token)
container = provider.get('MediaContainer', {}) if isinstance(provider, dict) else {}
channels = container.get('Channel', []) if isinstance(container, dict) else []
report = {'anonymous_registration_http': status, 'provider_http': provider_status,
          'channel_count': len(channels) if isinstance(channels, list) else 0,
          'identity': 'Plexium; no region spoofing or official-player impersonation',
          'metadata_status': 'available' if provider_status == 200 and isinstance(channels, list) and channels else 'not verified',
          'signed_in_profile_availability': 'not tested',
          'native_video_audio_ads_and_session_expiry': 'not tested'}
Path('artifacts').mkdir(exist_ok=True)
Path('artifacts/free-tv-compatibility.json').write_text(json.dumps(report, indent=2) + '\n')
print('::notice title=Plex Free TV compatibility report::' + json.dumps(report))
