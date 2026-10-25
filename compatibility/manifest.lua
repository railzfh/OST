-- Manifest request-code fallback for providers that remain reachable when
-- the built-in opensteamtool/wudrm/steamrun endpoints are unavailable.
-- Each response must be a single unsigned decimal string.

function fetch_manifest_code_ex(app_id, depot_id, gid)
    local function digits(body, status)
        if status ~= 200 or type(body) ~= "string" then return nil end
        return body:match("^%s*(%d+)%s*$")
    end

    local body, status = http_get(
        "https://manifest.manifestdex.com/" .. tostring(gid),
        { ["User-Agent"] = "ManifestDeX/1.0" }
    )
    local code = digits(body, status)
    if code then return code end

    body, status = http_get(
        "https://20770407.xyz/manifest/" .. tostring(depot_id) .. "/" .. tostring(gid),
        { ["User-Agent"] = "OpenSteamTool/1.0" }
    )
    return digits(body, status)
end

function fetch_manifest_code(gid)
    local function digits(body, status)
        if status ~= 200 or type(body) ~= "string" then return nil end
        return body:match("^%s*(%d+)%s*$")
    end

    local body, status = http_get(
        "https://manifest.manifestdex.com/" .. tostring(gid),
        { ["User-Agent"] = "ManifestDeX/1.0" }
    )
    return digits(body, status)
end
