import json

from .common import TestBase


class TestApi(TestBase):
    """Test /api get response."""

    def test(self):

        rv = self.client.get('/api/')

        expected = {
          "message": "Welcome to the Compute Web App",
          "status": "ok",
          "version": "1.0"
        }

        result = json.loads(rv.data)

        assert 'data_path' in result
        assert 'process_definition_path' in result
        assert 'result_path' in result

        del result['data_path']
        del result['process_definition_path']
        del result['result_path']

        assert result == expected
