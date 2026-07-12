-- Github_make: fetch and build a GitHub repo archive.
--
-- To avoid GitHub API rate limiting (60 req/hr unauthenticated),
-- authenticate via the GitHub CLI:
--
--   pacman -S github-cli
--   gh auth login
--
-- This reads your token via `gh auth token` and uses it for API
-- requests, raising the limit to 5000 req/hr. If `gh` is not
-- installed or not authenticated, requests are made without a token
-- (60 req/hr limit).

local function gh_token()
    local f = io.popen("gh auth token 2>/dev/null")
    if not f then return "" end
    local tok = f:read("*a"):gsub("%s+", "")
    f:close()
    return tok
end

local _gh_token = gh_token()
local _auth_flag = _gh_token ~= "" and "-H 'Authorization: token " .. _gh_token .. "'" or ""

function Github_make(user, repo, branch, artifact)
    return {
        url = "https://github.com/" .. user .. "/" .. repo .. "/archive/refs/heads/" .. branch .. ".tar.gz",
        name = repo .. ".tar.gz",
        last_modified_cmd = "curl -s " ..
        _auth_flag ..
        " https://api.github.com/repos/" .. user .. "/" .. repo .. "/commits/" .. branch .. " -I | grep -i last-modified",
        build = "tar xzf " .. repo .. ".tar.gz && cp -fr " .. repo .. "-" .. branch .. "/* . && make",
        artifact = artifact,
    }
end
