import requests

__all__ = [
    'Gitlab',
]


def _mkrequest(path, **defkwargs):
    def func(self, *args, **kwargs):
        data = defkwargs.copy()
        data.update(kwargs)

        return self.fetch(path.format(*args), **data)

    return func


def _mkrequest_paged(path, **defkwargs):
    def func(self, *args, **kwargs):
        data = defkwargs.copy()
        data.update(kwargs)

        return self.fetch_all(path.format(*args), **data)

    return func


class Gitlab(object):
    def __init__(self, host, token, use_ssl=True):
        self.urlbase = '%s://%s/api/v4' % ('https' if use_ssl else 'http', host)
        self.headers = {
            'PRIVATE-TOKEN': token,
        }

    def fetch(self, path, **kwargs):
        url = '%s/%s' % (self.urlbase, path)
        response = requests.get(url, headers=self.headers, params=kwargs)

        if response.status_code != 200:
            return False

        if callable(response.json):
            return response.json()
        else:
            return response.json

    def fetch_all(self, path, **kwargs):
        kwargs.update({
            'page': 1,
            'per_page': 100,
        })

        full = []
        while True:
            items = self.fetch(path, **kwargs)
            if not items:
                break

            full += items
            if len(items) < kwargs['per_page']:
                break
            kwargs['page'] += 1

        return full

    # Projects
    getproject = _mkrequest('projects/{}')
    getmilestones = _mkrequest('projects/{}/milestones')

    # Merge requests
    getmergerequests = _mkrequest_paged('projects/{}/merge_requests')
